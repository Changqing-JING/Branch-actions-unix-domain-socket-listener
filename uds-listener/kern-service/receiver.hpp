#pragma once

#include <atomic>
#include <condition_variable>
#include <iostream>
#include <map>
#include <mutex>
#include <queue>
#include <thread>

#include "../../common/interface.h"
#include "epollserver.h"
#include "eventloop.h"
#include "trustSocket.h"

class Data {
public:
  char process_name[sizeof(((serialize_socket_data *)0)->process_name)];
  char path[sizeof(((serialize_socket_data *)0)->path)];
  decltype(((serialize_socket_data *)0)->isRead) isRead;
  decltype(((serialize_socket_data *)0)->data_len) len;
  void *p_data;
  Data() : p_data(nullptr){};
  Data(serialize_socket_data &_p, void *_data);
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
    if (p_data != nullptr) {
      (std::free(p_data));
    }
  }

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
  std::thread m_eventloopThread;

  std::queue<Data> m_recv_messages;
  std::map<int32_t, std::string> m_connec_map;

public:
  Receiver() : m_eps(nullptr), m_status(ReceiverStatus::INIT) {
    m_eventloopThread = std::thread{[]() { Eventloop::getInstance().run(); }};
  }
  ~Receiver() {
    if (m_eventloopThread.joinable()) {
      m_eventloopThread.join();
    }
  }

  void start(uint16_t port);
  void stop() { m_status = ReceiverStatus::STOPING; }
  Data popMessage();
  bool canPopMessage() { return !m_recv_messages.empty(); }
  std::string getProcessName(int fd);

private:
  void serverloop();
  int onReceiveData(int fd);
};