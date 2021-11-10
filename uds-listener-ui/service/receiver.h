#pragma once

#include <condition_variable>
#include <cstring>
#include <mutex>
#include <queue>
#include <thread>
#include <unistd.h>

#include "epollserver.h"
#include "eventloop.h"
#include "trustSocket.h"

#pragma pack(1)
typedef struct socket_data {
  int fd;
  char path[108];
  int isRead;
  int data_len;
  char data[0];
} socket_data;
#pragma pack()

class Data {
public:
  int fd;
  char path[108];
  int isRead;
  int len;
  void *p_data;
  Data() = delete;
  Data(socket_data &_p, void *_data)
      : fd(_p.fd), isRead(_p.isRead), len(_p.data_len) {
    std::memcpy(path, _p.path, 108);
    p_data = std::malloc(_p.data_len);
    std::memcpy(p_data, _data, _p.data_len);
  }
  Data(Data const &a) { copy(a); }
  Data &operator=(Data const &a) {
    copy(a);
    return *this;
  }
  Data(Data &&a) { swap(std::move(a)); }
  Data &operator=(Data &&a) {
    swap(std::move(a));
    return *this;
  }

  ~Data() { std::free(p_data); }

private:
  void copy(Data const &a) {
    fd = a.fd;
    isRead = a.isRead;
    len = a.len;
    std::memcpy(path, a.path, 108);
    p_data = std::malloc(a.len);
    std::memcpy(p_data, a.p_data, a.len);
  }
  void swap(Data &&a) {
    fd = a.fd;
    isRead = a.isRead;
    len = a.len;
    std::memcpy(path, a.path, 108);
    std::swap(p_data, a.p_data);
  }
};

class Receiver {
  EpollServer m_eps;
  std::mutex m_mutex;
  std::condition_variable m_cv;
  std::queue<Data> m_recv_messages;

public:
  Receiver(uint16_t port) : m_eps(EpollServer{}) {
    Eventloop::getInstance().insert_job(0, [this, port]() {
      this->m_eps.init_server(
          port, [this](int fd) { return this->onReceiveData(fd); });
      this->serverloop();
    });
    std::thread{[]() { Eventloop::getInstance().run(); }};
  }

  Data get_message() {
    std::unique_lock<std::mutex> lo(m_mutex);
    m_cv.wait(lo, [this]() { return !this->m_recv_messages.empty(); });
    Data front = std::move(m_recv_messages.front());
    m_recv_messages.pop();
    return front;
  }

private:
  void serverloop() {
    m_eps.server_loop();
    Eventloop::getInstance().insert_job(1000, [this]() { this->serverloop(); });
  }

  int onReceiveData(int fd) {
    socket_data dataHeader;
    int len = trustRecv(fd, &dataHeader, sizeof(dataHeader), MSG_WAITALL);
    if (len == 0) {
      return CONNECT_ERROR;
    }
    void *buf = malloc(dataHeader.data_len);
    len = trustRecv(fd, buf, dataHeader.data_len, MSG_WAITALL);
    if (len == 0) {
      return CONNECT_ERROR;
    }
    free(buf);
    {
      std::lock_guard<std::mutex> lo(m_mutex);
      m_recv_messages.emplace(std::move(Data{dataHeader, buf}));
    }
    m_cv.notify_all();
    return CONNECT_OK;
  }
};