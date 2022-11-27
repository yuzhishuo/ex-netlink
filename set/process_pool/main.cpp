#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <any>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <sys/syscall.h>

pid_t gettid(void)
{
return syscall(SYS_gettid);
}

#include "Billboard.h"

class ProcessHandle {
 public:
  using process_exec = int(std::any&, Billboard&);

 public:
  ProcessHandle() = delete;
  ProcessHandle(const std::string& process_name,
                std::function<process_exec> pexec = nullptr)
      : exec_(pexec), process_name_{'\0', MAX_PROCESS_NAME} {
    for (size_t i = 0; i < std::min(MAX_PROCESS_NAME, process_name.size());
         i++) {
      process_name_ = process_name[i];
    }
  }

  const std::function<process_exec>& set_exec(
      std::function<process_exec>&& pexec) noexcept {
    exec_ = std::forward<std::function<process_exec>>(pexec);
    return pexec;
  }

  auto send(std::vector<uint32_t>& data) { billboard_->send(data); }
  auto recv(std::vector<uint32_t>& data) { billboard_->recv(data); }

  auto set_billboard(Billboard&& billboard) {
    billboard_ = std::make_unique<Billboard>(std::move(billboard));
  }

  auto& get_billboard() { return *billboard_; }
  auto invoke() -> int const {
    int res;
    try {
      res = this->exec_(this->user_data_, *billboard_);
    } catch (...) {
      printf("process %s generated exception", get_process_name().data());
      return -1;
    }
    return res;
  }

  template <class T>
  void set_user_data(T&& data) noexcept {
    user_data_ = std::move(data);
  }

  template <class T>
  T* get_user_data() noexcept {
    return std::any_cast<T>(&user_data_);
  }

  const std::string& get_process_name() const noexcept {
    return this->process_name_;
  }

 private:
  constexpr static size_t MAX_PROCESS_NAME = 20;
  std::function<ProcessHandle::process_exec> exec_;
  std::string process_name_;
  std::any user_data_;
  std::unique_ptr<Billboard> billboard_;
};

class ProcessPool {
 public:
  ProcessPool() : process_seq_{0}, process_{} {}

  std::shared_ptr<ProcessHandle> new_process(
      std::function<int(Billboard&)> invoke) {
    if (!is_vail() || process_seq_ >= 4) {
      return nullptr;
    }

    char process_name[32];
    sprintf(process_name, "process_poll_%d", process_seq_);

    auto handle = std::make_shared<ProcessHandle>(
        process_name,
        [fn = std::move(invoke)](std::any&, Billboard& billboard) -> int {
          return fn(billboard);
        });
    Billboard billboard;
    handle->set_billboard(std::move(billboard));

    if (auto child_id = fork(); child_id) {
      handle->get_billboard().start();
      process_.insert(std::make_pair(std::string(process_name), handle));
      return handle;
    }

    handle->get_billboard().start();
    exit(handle->invoke());
    // not arrived
    return nullptr;
  }

 private:
  static bool is_vail() { return current_thread_id == gettid(); }

 private:
  uint8_t process_seq_;
  std::unordered_map<std::string, std::shared_ptr<ProcessHandle>> process_;
};

int main(int argc, char const* argv[]) {
  pthread_atfork(NULL, NULL, []() { current_thread_id = gettid(); });
  ProcessPool processPool;
  processPool.new_process([](Billboard&) {
    printf("123\n");
    using namespace std::chrono;
    std::this_thread::sleep_for(5s);
    return 0;
  });

  processPool.new_process([](Billboard&) {
    printf("456\n");
    using namespace std::chrono;
    std::this_thread::sleep_for(5s);
    return 0;
  });

  processPool.new_process([](Billboard&) {
    printf("789\n");
    using namespace std::chrono;
    std::this_thread::sleep_for(5s);
    return 0;
  });

  processPool.new_process([](Billboard&) {
    printf("101112\n");
    using namespace std::chrono;
    std::this_thread::sleep_for(5s);
    return 0;
  });

  using namespace std::chrono;
  std::this_thread::sleep_for(10s);
  return 0;
}
