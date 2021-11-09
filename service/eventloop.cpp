#include "eventloop.h"
#include <ctime>
#include <iomanip>
#include <iostream>
#include <thread>

void noncancel() {
  // null
}

void Eventloop::insert_job(uint64_t time_us, CallBack cb) {
  insert_job(time_us, cb, noncancel);
}
void Eventloop::insert_job(uint64_t time_us, CallBack cb, CallBack onCancel) {
  m_jobs.emplace(
      std::make_tuple(TimeStandard{time_us} + m_loopTime, cb, onCancel));
}

TimeStandard Eventloop::singleLoop() {
  m_loopTime = std::chrono::duration_cast<TimeStandard>(
      steady_clock::now().time_since_epoch());
  while (!m_jobs.empty()) {
    auto job = m_jobs.top();
    if (std::get<0>(job) > m_loopTime) {
      return std::chrono::duration_cast<TimeStandard>(std::get<0>(job) -
                                                      m_loopTime);
    }
    m_jobs.pop();
    std::get<1>(job)();
  }
  return m_defaultStep;
}

void Eventloop::run() {
  for (;;) {
    TimeStandard sleepTime = singleLoop();
    std::this_thread::sleep_for(sleepTime);
  }
}

void Eventloop::stop() {
  // todo : need multi-thread
}