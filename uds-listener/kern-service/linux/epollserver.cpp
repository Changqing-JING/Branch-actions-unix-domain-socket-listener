#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdexcept>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "epollserver.h"

Server::Server() : m_listen_fd(-1), m_connect_fds({}) {
  m_epoll_fd = epoll_create(1);
  if (m_epoll_fd == -1) {
    perror("epoll create");
    throw std::runtime_error("epoll create failed");
  }
}

Server::~Server() {
  clear_fd(m_listen_fd);
  for (auto fd : m_connect_fds) {
    clear_fd(fd);
  }
  close(m_epoll_fd);
}

void Server::init_server(uint16_t port, std::function<int(int)> on_recv_data) {
  // set call back
  m_on_recv_data = on_recv_data;
  // init listener
  m_listen_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  Server::set_nonblock(m_listen_fd);
  sockaddr_in sockAddr{};
  sockAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  sockAddr.sin_family = AF_INET;
  sockAddr.sin_port = htons(port);
  if (-1 == bind(m_listen_fd, reinterpret_cast<sockaddr *>(&sockAddr),
                 sizeof(sockaddr_in))) {
    perror("bind");
    throw std::runtime_error("init failed");
  }
  if (-1 == listen(m_listen_fd, 10)) {
    perror("listen");
    throw std::runtime_error("init failed");
  }

  epoll_event epollEvent{};
  epollEvent.events = EPOLLIN;
  epollEvent.data.fd = m_listen_fd;
  if (-1 == epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, m_listen_fd, &epollEvent)) {
    perror("epoll add");
    throw std::runtime_error("init failed");
  }
}

void Server::server_loop() {
  int eventCount = epoll_wait(m_epoll_fd, m_events, MAX_EPOLL_EVENT, 0);
  if (eventCount == -1) {
    perror("epoll wait");
    throw std::runtime_error("epoll wait");
  }
  for (int i = 0; i < eventCount; i++) {
    epoll_event event = m_events[i];
    // listen fd + EPOLLIN => new client
    if (event.data.fd == m_listen_fd) {
      if ((event.events & EPOLLIN)) {
        this->accept_new_client();
      }
      continue;
    }
    // other socked
    if (event.events & EPOLLIN) {
      if (CONNECT_ERROR == m_on_recv_data(event.data.fd)) {
        this->remove_client(event.data.fd);
      }
      continue;
    }
  }
}

void Server::accept_new_client() {
  sockaddr_in cli_addr{};
  uint32_t length = sizeof(cli_addr);
  int fd =
      accept(m_listen_fd, reinterpret_cast<sockaddr *>(&cli_addr), &length);
  if (fd == -1) {
    perror("accept");
    throw std::runtime_error("accept");
  }
  epoll_event epollEvent{};
  // epollEvent.events = EPOLLIN | EPOLLET;
  epollEvent.events = EPOLLIN;
  epollEvent.data.fd = fd;
  Server::set_nonblock(fd);
  epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, fd, &epollEvent);
  m_connect_fds.insert(fd);
}

void Server::clear_fd(int fd) {
  epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
  if (-1 == close(fd)) {
    perror("close");
    throw std::runtime_error("close fd");
  }
}

void Server::remove_client(int fd) {
  clear_fd(fd);
  m_connect_fds.erase(fd);
}

void Server::set_nonblock(int fd) {
  int flag = fcntl(fd, F_GETFL);
  if (flag == -1) {
    perror("fcntl");
    throw std::runtime_error("new client fd operation failed");
  }
  if (-1 == fcntl(fd, F_SETFL, flag | O_NONBLOCK)) {
    perror("fcntl");
    throw std::runtime_error("new client fd operation failed");
  }
}