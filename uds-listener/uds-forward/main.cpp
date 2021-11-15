#include <fstream>
#include <getopt.h>

#include "../kern-service/receiver.hpp"

constexpr int STORE_GAP = 1000;

std::shared_ptr<std::string> hexData(void *data, int len) {
  std::cout << "hexData len = " << len << std::endl;
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

void store(Receiver &recv, std::ofstream &outputfile) {
  std::cout << "store - " << recv.canPopMessage() << "\n";
  while (recv.canPopMessage()) {
    Data tmp = recv.popMessage();
    outputfile << tmp.process_name << "\t";
    outputfile << tmp.path << "\t";
    outputfile << (tmp.isRead ? "R" : "W") << "\t";
    outputfile << hexData(tmp.p_data, tmp.len)->data() << "\t";
    outputfile << strData(tmp.p_data, tmp.len)->data() << "\n";
  }
  outputfile.flush();
  Eventloop::getInstance().insert_job(
      STORE_GAP, [&recv, &outputfile]() { store(recv, outputfile); });
}

int main(int argc, char *argv[]) {
  int in_port = 12345;
  int out_port = 12346; // todo
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
  Receiver recv{};

  Eventloop::getInstance().insert_job(
      STORE_GAP, [&recv, &outputfile]() { store(recv, outputfile); });
  recv.start(static_cast<uint16_t>(in_port));
  std::cout << "start server in " << in_port << std::endl;
}