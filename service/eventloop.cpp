#include "eventloop.h"
#include <ctime>
#include <iomanip>
#include <iostream>
#include <thread>

Eventloop *Eventloop::ins = nullptr;

void noncancel() {
  // null
}

// void print_time(MicroTime t) {
//     const auto t_c = t.time_since_epoch();
//     std::cout << t_c.count() << std::endl;
// }

inline MicroTime modify_timer(const MicroTime &t, double dt_ms) {
  return t +
         std::chrono::duration_cast<
             std::chrono::duration<long int, std::micro>>(MillDuration(dt_ms));
}

void Eventloop::insert_job(double time_ms, CallBack cb) {
  m_jobs.emplace(
      std::make_tuple(modify_timer(m_loopTime, time_ms), cb, noncancel));
}

void Eventloop::insert_job(double time_ms, CallBack cb, CallBack onCancel) {
  m_jobs.emplace(
      std::make_tuple(modify_timer(m_loopTime, time_ms), cb, onCancel));
}

MicroDuration Eventloop::_loop() {
  m_loopTime = std::chrono::time_point_cast<std::chrono::microseconds>(
      steady_clock::now());
  while (!m_jobs.empty()) {
    std::cout << "m_job nums: " << m_jobs.size() << std::endl;
    auto job = m_jobs.top();
    if (std::get<0>(job) > m_loopTime) {
      return std::chrono::duration_cast<MicroDuration>(std::get<0>(job) -
                                                       m_loopTime);
    }
    m_jobs.pop();
    std::get<1>(job)();
  }
  return m_defaultStep;
}

void Eventloop::run() {
  for (;;) {
    const MicroDuration sleepTime = _loop();
    std::cout << "sleep: " << sleepTime.count() / 1000 << "ms" << std::endl;
    std::this_thread::sleep_for(sleepTime);
  }
}

void Eventloop::stop() {}