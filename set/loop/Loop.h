#pragma once

#ifndef YUZHI_TOOL_LOOP_H
#define YUZHI_TOOL_LOOP_H

#include <algorithm>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <ranges>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>
namespace luluyuzhi::net {
class IHandle;
}
namespace luluyuzhi::tool {

class Thread;

struct timer {
  uint64_t timestamp;
  std::function<void()> runnable;

  auto operator<=>(const timer& t) { return this->timestamp <=> t.timestamp; }

  auto operator<(const timer& t) { return this->timestamp < t.timestamp; }
  auto operator==(const timer& t) { return this->timestamp == t.timestamp; }
  friend bool operator<(const timer& l, const timer& r);
};

inline bool operator>(const timer& l, const timer& r) {
  return l.timestamp > r.timestamp;
}

class Loop {
 public:
  Loop();
  ~Loop();
  void run();
  void quit() noexcept;
  void runAt(uint64_t timestamp, std::function<void()> func) noexcept;
  void pushIdle(std::function<void()> runnable) noexcept;
  void pushRunable(std::function<void()> runnable) noexcept;
  void registerIoHandle(std::shared_ptr<net::IHandle> handle) noexcept;

 private:
  void handle_runnable(std::vector<std::function<void()>>& runnables,
                       std::mutex& mut);
  void handle_timer(std::priority_queue<timer, std::vector<timer>,
                                        std::greater<timer>>& timer,
                    uint64_t timestamp);
  uint64_t get_outtime() const;
  inline void handle_io_source(uint64_t outtime);

 private:
  bool is_quit_;

  std::vector<std::function<void()>> idle_;
  std::vector<std::function<void()>> runable_;
  std::priority_queue<timer, std::vector<timer>, std::greater<timer>> timer_;
  std::mutex run_mut_;
  std::mutex idle_mut_;
  std::mutex time_mut_;

  std::shared_ptr<net::IHandle> io_handle_;
};

}  // namespace luluyuzhi::tool

#endif  // YUZHI_TOOL_LOOP_H