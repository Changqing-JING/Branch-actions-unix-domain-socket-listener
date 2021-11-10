#pragma once

#include <condition_variable>
#include <cstring>
#include <iostream>
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
  Data(Data &&a) { move(std::move(a)); }
  Data &operator=(Data &&a) {
    move(std::move(a));
    return *this;
  }

  ~Data() {
    // if (p_data != nullptr) {
    std::free(p_data);
    // }
  }

private:
  void copy(Data const &a) {
    fd = a.fd;
    isRead = a.isRead;
    len = a.len;
    std::memcpy(path, a.path, 108);
    p_data = std::malloc(a.len);
    std::memcpy(p_data, a.p_data, a.len);
  }
  void move(Data &&a) {
    fd = a.fd;
    isRead = a.isRead;
    len = a.len;
    std::memcpy(path, a.path, 108);
    p_data = a.p_data;
    a.p_data = nullptr;
  }
};

enum ReceiverStatus : int8_t { INIT, START, STOPING, STOP };

class Receiver {
  EpollServer *m_eps;
  std::mutex m_mutex;
  std::atomic<ReceiverStatus> m_status;
  std::condition_variable m_cv;
  std::queue<Data> m_recv_messages;
  std::thread m_eventloopThread;

public:
  Receiver() : m_eps(nullptr), m_status(ReceiverStatus::INIT) {
    m_eventloopThread = std::thread{[]() { Eventloop::getInstance().run(); }};
  }

  void start(uint16_t port) {
    {
      std::lock_guard<std::mutex> lo(m_mutex);
      if (m_status == ReceiverStatus::INIT ||
          m_status == ReceiverStatus::STOP) {
        m_eps = new EpollServer{};
        m_eps->init_server(port,
                           [this](int fd) { return this->onReceiveData(fd); });
      } else if (m_status == ReceiverStatus::STOPING) {
      } else {
        throw std::runtime_error("start receiver under error status");
      }
      m_status = ReceiverStatus::START;
    }

    Eventloop::getInstance().insert_job(0,
                                        [this, port]() { this->serverloop(); });
  }

  void stop() { m_status = ReceiverStatus::STOPING; }

  Data popMessage() {
    std::unique_lock<std::mutex> lo(m_mutex);
    m_cv.wait(lo, [this]() { return !this->m_recv_messages.empty(); });
    Data front = std::move(m_recv_messages.front());
    m_recv_messages.pop();
    return front;
  }

  bool canPopMessage() { return !m_recv_messages.empty(); }

private:
  void serverloop() {
    m_eps->server_loop();
    if (m_status != ReceiverStatus::STOPING) {
      Eventloop::getInstance().insert_job(1000,
                                          [this]() { this->serverloop(); });
    } else {
      std::lock_guard<std::mutex> lo{m_mutex};
      delete m_eps;
      m_status = ReceiverStatus::STOP;
    }
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
      free(buf);
      return CONNECT_ERROR;
    }
    {
      std::lock_guard<std::mutex> lo(m_mutex);
      m_recv_messages.emplace(std::move(Data{dataHeader, buf}));
    }
    free(buf);
    m_cv.notify_all();
    return CONNECT_OK;
  }
};