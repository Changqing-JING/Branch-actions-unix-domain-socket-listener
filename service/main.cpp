#include <iostream>
#include <unistd.h>

#include "eventloop.h"
#include "server.h"

const int MAX_DATA_LEN = 8192;

int on_recv_data(int fd) {
  char buffer[MAX_DATA_LEN];
  int len = read(fd, buffer, MAX_DATA_LEN);
  if (len == -1) {
    return -1;
  }
  buffer[len] = '\0';
  std::cout << "get data from " << fd << ", data: \n"
            << std::string(buffer) << std::endl;
  if (len == MAX_DATA_LEN) {
    Eventloop::getInstance()->insert_job(10, std::bind(&on_recv_data, fd));
  }
  return 0;
}

void server_loop(EpollServer *eps) {
  eps->server_loop();
  Eventloop::getInstance()->insert_job(1000, std::bind(&server_loop, eps));
}

void initEpollServer(uint16_t port) {
  EpollServer *eps = new EpollServer();
  eps->init_server(port, on_recv_data);
  Eventloop::getInstance()->insert_job(0, std::bind(&server_loop, eps));
}

int main(void) {
  try {
    Eventloop *ev = Eventloop::getInstance();
    ev->insert_job(0, std::bind(&initEpollServer, 12345));
    ev->run();
  } catch (std::exception e) {
    std::cout << e.what() << std::endl;
  }
}