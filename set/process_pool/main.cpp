#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <any>
#include <ranges>
#include <string>
#include <thread>
#include <type_traits>
#include <unordered_map>
  
static std::thread::id current_thread_id;
class Process {
 public:
  using process_exec = int (*)(void);

 public:
  Process() = delete;
  Process(std::string process_name, process_exec pexec = nullptr)
      : exec_(pexec), process_name_{'\0', MAX_PROCESS_NAME} {
    for (size_t i = 0; i < std::min(MAX_PROCESS_NAME, process_name.size());
         i++) {
      process_name_ = process_name[i];
    }
  }

  process_exec set_exec(process_exec pexec) {
    std::swap(pexec, exec_);
    return pexec;
  }

  template <class T>
  void set_user_data(T data) {
    user_data_ = std::move(data);
  }

  template <class T>
  T* get_user_data() {
    return std::any_cast<T>(&user_data_);
  }

  const std::string& get_process_name() const&& { return this->process_name_; }

 private:
  constexpr static size_t MAX_PROCESS_NAME = 20;
  process_exec exec_;
  std::string process_name_;
  std::any user_data_;
};

class ProcessPool {
 public:
  ProcessPool()
  {
    std::this_thread::get_id();
  }

 private:
  std::unordered_map<std::string, Process> process_;
};
int main(int argc, char const* argv[]) 
{
  /* code */
  return 0;
}
