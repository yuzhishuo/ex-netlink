#ifndef YUZHI_THREAD_POOL_H
#define YUZHI_THREAD_POOL_H

#include <atomic>
#include <condition_variable>
#include <deque>
#include <functional>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

class thread_pool {
 public:
  thread_pool() : data_{std::make_shared<data>()} {
    for (size_t i = 0; i < 4; i++) {
      trds_.emplace_back([_d = data_]() {
        try {
          while (true) { // while (_d->stoped) { // why not here?
            std::function<void()> ts;
            std::unique_lock<std::mutex> lock(_d->mut_);
            if (!_d->task.empty()) {
              ts = _d->task.front();
              _d->task.pop_front();
              lock.unlock();
              try {
                ts();
              } catch (...) {
                std::cout << "generate error" << std::endl;
              }
            } else if (_d->stoped) { // Attent code location
              break;
            } else {
              _d->cond_.wait(lock);
            }
          }
        } catch (...) {
          std::cout << "generr" << std::endl;
        }
      });
      std::cout << trds_.back().joinable() << std::endl;
    }
  }

  ~thread_pool() {}

  void push(std::function<void()>&& ts) {
    {
      std::unique_lock<std::mutex> lock(data_->mut_);
      data_->task.emplace_back(ts);
    }
    data_->cond_.notify_one();
  }

  void stop() {
    if (data_) {
      {
        std::unique_lock<std::mutex> lock(data_->mut_);
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
  std::vector<std::jthread> trds_;
  std::shared_ptr<data> data_;
};

#endif  // YUZHI_THREAD_POOL_H