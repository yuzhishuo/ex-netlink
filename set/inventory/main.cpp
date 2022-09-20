#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

void ssleep(size_t m) {
  timeval val;
  bzero(&val, sizeof(timeval));
  val.tv_sec = m;
  select(0, NULL, NULL, NULL, &val);
}

class Request {
 public:
  Request();
  void print();
  ~Request();

 private:
  mutable std::mutex mut_;
};

class Inventory {
 public:
  void register1(Request *request) {
    std::unique_lock lock(mut_);
    obs_.emplace_back(request);
  }
  void remove(Request *request) {
    std::unique_lock lock(mut_);
    std::erase_if(obs_, [&](Request *e) { return request == e; });  // c++ 20
  }
  void printAll() const {
    std::unique_lock lock(mut_);
    for (auto e : obs_) {
      e->print();
    }
  }

 private:
  mutable std::mutex mut_;
  std::vector<Request *> obs_;
};

Inventory inventory;

Request::Request() {
  std::unique_lock lock(mut_);
  inventory.register1(this);
}

void Request::print() {
  std::unique_lock lock(mut_);
  char buf[] = "Request\n";
  write(STDIN_FILENO, buf, sizeof(buf));
}

Request::~Request() {
  std::unique_lock lock(mut_);
  ssleep(5);
  inventory.remove(this);
}

int main(int argc, char const *argv[]) {
  Request e;
  std::thread t([&]() {
    ssleep(2);
    inventory.printAll();
  });
  e.~Request();
  t.join();

  return 0;
}
