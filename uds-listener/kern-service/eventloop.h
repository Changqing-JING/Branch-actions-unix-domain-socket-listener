#pragma 1

#include <chrono>
#include <functional>
#include <queue>
#include <ratio>
#include <stdint.h>
#include <tuple>

using CallBack = std::function<void()>;
using steady_clock = std::chrono::steady_clock;
using TimeStandard = std::chrono::milliseconds;
using Task = std::tuple<TimeStandard, CallBack, CallBack>;

struct greater {
  bool operator()(Task &a, Task &b) { return std::get<0>(a) >= std::get<0>(b); }
};

class Eventloop {
private:
  std::priority_queue<Task, std::vector<Task>, greater> m_jobs;
  TimeStandard m_defaultStep;
  TimeStandard m_loopTime;

public:
  static Eventloop &getInstance() {
    static Eventloop ins{};
    return ins;
  }

  void setDefaultStep(uint64_t delta_time_us) {
    m_defaultStep = TimeStandard{delta_time_us};
  }

  void insert_job(uint64_t time_us, CallBack cb);
  void insert_job(uint64_t time_us, CallBack cb, CallBack onCancel);
  void run();
  void stop();

private:
  TimeStandard singleLoop();
  Eventloop() : m_defaultStep(TimeStandard{100}){};
  ~Eventloop(){};
  Eventloop(const Eventloop &) = delete;
  Eventloop &operator=(const Eventloop &) = delete;
};