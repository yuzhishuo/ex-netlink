#pragma once
#ifndef YUZHI_THREAD_POOL_H
#define YUZHI_THREAD_POOL_H
#include <pthread.h>
#include <sys/types.h>

#include <any>
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <functional>
#include <iostream>
#include <mutex>
#include <string_view>
#include <thread>
#include <vector>

#include "Count.h"

namespace luluyuzhi::tool {

class ThreadPool {
 public:
  ThreadPool() : data_{std::make_shared<data>()} {
    max_thread_nums_ = std::thread::hardware_concurrency();
    if (max_thread_nums_ < 1) max_thread_nums_ = 4;
    for (size_t i = 0; i < max_thread_nums_; i++) {
      trds_.emplace_back(&ThreadPool::bind_run, data_);
    }
  }

  ~ThreadPool() {}

  void push(std::function<void()>&& ts) {
    {
      std::lock_guard<std::mutex> lock(data_->mut_);
      data_->task.emplace_back(std::move(ts));
    }
    data_->cond_.notify_one();
  }

  void stop() {
    if (data_) {
      {
        std::lock_guard<std::mutex> lock(data_->mut_);
        data_->stoped = true;
      }
      data_->cond_.notify_all();
    }
  }

 private:
  struct data {
    bool stoped = false;
    std::deque<std::function<void()>> task;
    std::mutex mut_;
    std::condition_variable cond_;
  };

  static void bind_run(std::shared_ptr<data> data) {
    try {
      while (true) {  // while (_d->stoped) { // why not here?
        std::function<void()> ts;
        std::unique_lock<std::mutex> lock(data->mut_);
        if (!data->task.empty()) {
          ts = std::move(data->task.front());
          data->task.pop_front();
          lock.unlock();
          try {
            ts();
          } catch (...) {
            std::cout << "generate error" << std::endl;
          }
        } else if (data->stoped) {  // Attent code location
          break;
        } else {
          data->cond_.wait(lock);
        }
      }
    } catch (...) {
      std::cout << "generr" << std::endl;
    }
  }

 private:
  uint64_t max_thread_nums_;
  std::vector<std::thread> trds_;
  std::shared_ptr<data> data_;
};

}  // namespace luluyuzhi::tool
#endif  // YUZHI_THREAD_POOL_H