#pragma 1

#include <chrono>
#include <functional>
#include <queue>
#include <ratio>
#include <stdint.h>
#include <tuple>

using CallBack = std::function<void()>;
using steady_clock = std::chrono::steady_clock;
using MicroTime = std::chrono::time_point<std::chrono::steady_clock,
                                          std::chrono::microseconds>;
using MicroDuration = std::chrono::duration<double, std::micro>;
using MillDuration = std::chrono::duration<double, std::milli>;

struct greater {
  bool operator()(std::tuple<MicroTime, CallBack, CallBack> &a,
                  std::tuple<MicroTime, CallBack, CallBack> &b) {
    return std::get<0>(a) >= std::get<0>(b);
  }
};

class Eventloop {
private:
  static Eventloop *ins;
  std::priority_queue<std::tuple<MicroTime, CallBack, CallBack>,
                      std::vector<std::tuple<MicroTime, CallBack, CallBack>>,
                      greater>
      m_jobs;
  MicroTime m_loopTime;
  MicroDuration m_defaultStep;

public:
  static Eventloop *getInstance() {
    if (Eventloop::ins == nullptr) {
      Eventloop::ins = new Eventloop();
      Eventloop::ins->m_loopTime =
          std::chrono::time_point_cast<std::chrono::microseconds>(
              steady_clock::now());
    }
    return Eventloop::ins;
  }
  static void setDefaultStep(double dt) {
    getInstance()->m_defaultStep = MicroDuration(dt);
  }

  void insert_job(double time_ms, CallBack cb);
  void insert_job(double time_ms, CallBack cb, CallBack onCancel);
  void run();
  void stop();

private:
  MicroDuration _loop();

  Eventloop() : m_defaultStep(MicroDuration(1000)){};
  ~Eventloop(){};
  Eventloop(const Eventloop &);
  Eventloop &operator=(const Eventloop &);
};