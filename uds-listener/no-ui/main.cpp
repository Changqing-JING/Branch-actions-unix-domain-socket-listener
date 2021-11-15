#include <fstream>

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

int main() {
  std::ofstream outputfile("log.txt");
  if (!outputfile.is_open()) {
    throw std::runtime_error("output file failed");
  }
  Receiver recv{};

  Eventloop::getInstance().insert_job(
      STORE_GAP, [&recv, &outputfile]() { store(recv, outputfile); });
  recv.start(12345);
  std::cout << "start server\n";
}