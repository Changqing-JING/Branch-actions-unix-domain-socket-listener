#pragma once

#include <atomic>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>

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
  Data(socket_data &_p, void *_data);
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

  ~Data() { std::free(p_data); }

private:
  void copy(Data const &a);
  void move(Data &&a);
};

enum ReceiverStatus : int8_t { INIT, START, STOPING, STOP };

class Receiver {
  EpollServer *m_eps;
  std::mutex m_mutex;
  std::atomic<int8_t> m_status;
  std::condition_variable m_cv;
  std::queue<Data> m_recv_messages;
  std::thread m_eventloopThread;

public:
  Receiver() : m_eps(nullptr), m_status(ReceiverStatus::INIT) {
    m_eventloopThread = std::thread{[]() { Eventloop::getInstance().run(); }};
  }

  void start(uint16_t port);
  void stop() { m_status = ReceiverStatus::STOPING; }
  Data popMessage();
  bool canPopMessage() { return !m_recv_messages.empty(); }

private:
  void serverloop();
  int onReceiveData(int fd);
};