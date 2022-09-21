#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <any>
#include <functional>
#include <string>
#include <thread>
#include <type_traits>
#include <unordered_map>
#include <utility>

static std::thread::id current_thread_id;
class Process {
 public:
  using process_exec = int(std::any&);

 public:
  Process() = delete;
  Process(const std::string& process_name,
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

  auto invoke() -> int const {
    int res;
    try {
      res = this->exec_(this->user_data_);
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
  std::function<Process::process_exec> exec_;
  std::string process_name_;
  std::any user_data_;
};

class ProcessPool {
 public:
  ProcessPool() : process_seq_{0}, process_{} {
    current_thread_id = std::this_thread::get_id();
  }

  bool new_process(std::function<int()> invoke) {
    if (!is_vail() || process_seq_ >= 4) {
      return false;
    }

    char process_name[32];
    sprintf(process_name, "process_poll_%d", process_seq_);

    Process process(process_name, [=](std::any&) -> int { return invoke(); });

    if (auto child_id = fork(); child_id) {
      return true;
    }

    exit(process.invoke());
    // not arrived
    return true;
  }

 private:
  static bool is_vail() {
    return current_thread_id == std::this_thread::get_id();
  }

 private:
  uint8_t process_seq_;
  std::unordered_map<std::string, Process, std::less<>> process_;
};
int main(int argc, char const* argv[]) {
  ProcessPool processPool;
  processPool.new_process([]() {
    printf("123\n");
    using namespace std::chrono;
    std::this_thread::sleep_for(5s);
    return 0;
  });

  processPool.new_process([]() {
    printf("456\n");
    using namespace std::chrono;
    std::this_thread::sleep_for(5s);
    return 0;
  });

  using namespace std::chrono;
  std::this_thread::sleep_for(10s);
  return 0;
}
