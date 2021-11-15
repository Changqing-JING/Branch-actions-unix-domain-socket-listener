#pragma once

#include <atomic>
#include <condition_variable>
#include <iostream>
#include <map>
#include <mutex>
#include <queue>
#include <thread>

#include "../../common/interface.h"
#include "./linux/epollserver.h"
#include "./linux/eventloop.h"
#include "data.h"

enum ReceiverStatus : int8_t { INIT, START, STOPING, STOP };

class Receiver {
  Server *m_eps;
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