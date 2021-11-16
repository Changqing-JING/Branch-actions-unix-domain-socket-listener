#include <cstring>
#include <fstream>
#include <getopt.h>
#include <iostream>
#include <mutex>
#include <netinet/in.h>
#include <set>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

#include "../kern-service/receiver.hpp"

class ForwardServer {
  int listen_fd;
  std::thread listen_th;
  std::set<int> client_fds;
  std::mutex set_mutex;

public:
  ForwardServer(int port) {
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd == -1) {
      perror("socket");
      throw std::runtime_error("socket failed");
    }
    struct sockaddr_in localaddr;
    std::memset(&localaddr, 0, sizeof(localaddr));
    localaddr.sin_family = AF_INET;
    localaddr.sin_port = htons(port);
    localaddr.sin_addr.s_addr = INADDR_ANY;
    int ret = bind(listen_fd, (struct sockaddr *)&localaddr, sizeof(localaddr));
    if (ret == -1) {
      perror("bind");
      throw std::runtime_error("bind failed");
    }
    ret = listen(listen_fd, 10);
    if (ret == -1) {
      perror("listen");
      throw std::runtime_error("listen failed");
    }
  }
  ~ForwardServer() {
    if (listen_th.joinable()) {
      listen_th.join();
    }
    if (listen_fd != -1) {
      close(listen_fd);
    }
  }
  void start() {
    listen_th = std::thread{[this]() {
      for (;;) {
        std::cout << "accept\n";
        struct sockaddr_in remoteaddr;
        socklen_t addr_len = sizeof(struct sockaddr);
        int accept_fd =
            ::accept(listen_fd, (struct sockaddr *)&remoteaddr, &addr_len);
        std::cout << "connected successfully\n";
        std::lock_guard<std::mutex> lo{set_mutex};
        client_fds.insert(accept_fd);
      }
    }};
  }
  void send(std::string const &data) {
    std::lock_guard<std::mutex> lo{set_mutex};
    std::vector<int> error_fds{};
    for (auto fd : client_fds) {
      std::cout << "output " << fd << " " << data << std::endl;
      uint32_t len = htonl(data.length());
      int ret = ::send(fd, &len, sizeof(len), MSG_NOSIGNAL);
      if (ret < sizeof(len)) {
        perror("send");
        error_fds.push_back(fd);
        continue;
      }
      ret = ::send(fd, data.data(), data.length(), MSG_NOSIGNAL);
      if (ret < data.length()) {
        perror("send");
        error_fds.push_back(fd);
        continue;
      }
    }
    for (auto fd : error_fds) {
      client_fds.erase(fd);
      close(fd);
    }
  }
};

constexpr int STORE_GAP = 1000;

std::shared_ptr<std::string> hexData(void *data, int len) {
  if (len == 0 || data == nullptr) {
    return std::make_shared<std::string>("");
  }
  auto res = std::make_shared<std::string>();
  for (int i = 0; i < len; i++) {
    char t[3];
    sprintf(t, "%02x", static_cast<uint8_t *>(data)[i]);
    res->push_back(t[0]);
    res->push_back(t[1]);
    res->push_back(' ');
  }
  return res;
}
std::shared_ptr<std::string> strData(void *data, int len) {
  if (len == 0 || data == nullptr) {
    return std::make_shared<std::string>("");
  }
  auto res = std::make_shared<std::string>();
  for (int i = 0; i < len; i++) {
    int ascii = static_cast<uint8_t *>(data)[i];
    res->push_back(ascii > 20 ? ascii : 46);
  }
  return res;
}

void output(Receiver &recv, std::ofstream &outputfile,
            ForwardServer &outputserver) {
  while (recv.canPopMessage()) {
    Data tmp = recv.popMessage();
    using std::string;
    string outputmsg{string(tmp.process_name) + string{"\t"} +
                     string{tmp.path} + string{"\t"} +
                     string{(tmp.isRead ? "R" : "W")} + string{"\t"} +
                     *hexData(tmp.p_data, tmp.len) + string{"\t"} +
                     *strData(tmp.p_data, tmp.len)};
    outputfile << outputmsg << "\n";
    outputserver.send(outputmsg);
  }
  outputfile.flush();
  Eventloop::getInstance().insert_job(STORE_GAP,
                                      [&recv, &outputfile, &outputserver]() {
                                        output(recv, outputfile, outputserver);
                                      });
}

int main(int argc, char *argv[]) {
  int in_port = 12345;
  int out_port = 22222;
  int out_file_argv_index = -1;

  const struct option long_options[] = {{"in", required_argument, &in_port, 0},
                                        {"out", required_argument, NULL, 0},
                                        {"file", required_argument, NULL, 0},
                                        {0, 0, 0, 0}};

  int opt_index;
  int opt;
  while (getopt_long(argc, argv, "", long_options, &opt_index) != -1) {
    switch (opt_index) {
    case 0:
      in_port = std::stoi(argv[optind - 1]);
      break;
    case 1:
      out_port = std::stoi(argv[optind - 1]);
      break;
    case 2:
      out_file_argv_index = optind - 1;
      break;
    default:
      throw std::runtime_error("invaild argument");
    }
  }

  std::ofstream outputfile{
      out_file_argv_index == -1 ? "log.txt" : argv[out_file_argv_index]};
  if (!outputfile.is_open()) {
    throw std::runtime_error("output file failed");
  }

  ForwardServer outputserver{out_port};

  Receiver recv{};

  Eventloop::getInstance().insert_job(
      STORE_GAP, [&recv, &outputfile, &outputserver, in_port, out_port]() {
        recv.start(static_cast<uint16_t>(in_port));
        std::cout << "start input server in " << in_port << std::endl;
        outputserver.start();
        std::cout << "start output server in " << out_port << std::endl;
        output(recv, outputfile, outputserver);
      });
  std::this_thread::sleep_until(
      std::chrono::system_clock::now() +
      std::chrono::hours(std::numeric_limits<int>::max()));
}