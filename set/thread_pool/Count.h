#pragma once
#ifndef YUZHI_THREAD_COUNT_H
#define YUZHI_THREAD_COUNT_H

#include <condition_variable>
#include <cstdint>
#include <mutex>
namespace luluyuzhi::tool {

template <uint16_t count>
class Count {
 public:
  Count() : count_(count) {}
  void down() {
    std::unique_lock lock(mut_);
    count_--;
    if (count_ == 0) con_.notify_all();
  }

  void wait() noexcept {
    std::unique_lock lock(mut_);
    con_.wait(lock, [this]() -> bool { return count_ == 0; });
  }

 private:
  uint16_t count_;
  std::mutex mut_;
  std::condition_variable con_;
};

using once = Count<1>;

}  // namespace luluyuzhi::tool

#endif  // YUZHI_THREAD_COUNT_H