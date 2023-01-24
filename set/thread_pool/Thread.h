#pragma once
#ifndef YUZHI_THREAD_THREAD_H
#define YUZHI_THREAD_THREAD_H
#include <pthread.h>

#include <any>
#include <functional>
#include <iostream>
#include <string>
#include <thread>

#include "Count.h"

namespace luluyuzhi::tool {

class Thread {
 public:
  Thread(std::function<void(Thread*, std::any)> func, std::any data,
         std::string name)
      : name_{name}, thread_(std::bind(&Thread::run, this, func, data)) {
    once_.wait();
  }

  Thread(const Thread&) = delete;
  ~Thread() {
    if (thread_.joinable()) thread_.join();
  }

  std::string_view getThreadName() const { return name_; }

 private:
  void run(std::function<void(Thread*, std::any)> func,
           std::any data) noexcept {
    set_thread_name();
    once_.down();

    try {
      if (func) func(this, data);
    } catch (...) {
      // FIXME
      std::cout << "cat exception" << std::endl;
    }
  }

  void set_thread_name() noexcept {
    if (name_.empty()) return;
    auto native_hanle = thread_.native_handle();
    pthread_setname_np(native_hanle, name_.data());
  }

 private:
  std::string name_;
  mutable once once_;
  std::thread thread_;
};

}  // namespace luluyuzhi::tool

#endif  // YUZHI_THREAD_THREAD_H