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

class Request : public std::enable_shared_from_this<Request> {
 public:
  Request();
  void print();
  void register1();
  ~Request();

 private:
  mutable std::mutex mut_;
};

class Inventory {
 public:
  void register1(std::weak_ptr<Request> request) {
    std::unique_lock lock(mut_);
    obs_.emplace_back(request);
  }
  void remove(Request* request) {
    std::unique_lock lock(mut_);
    std::erase_if(obs_, [&](std::weak_ptr<Request> e) {
      if (auto r = e.lock(); r)
        return request == r.get();
      else {
        return false;
      }
    });  // c++ 20
  }
  void printAll() const {
    std::vector<std::weak_ptr<Request>> obs;

    {
      std::unique_lock lock(mut_);
      obs = obs_;
    }
    for (auto e : obs) {
      if (auto r = e.lock(); r) r->print();
    }
  }

 private:
  mutable std::mutex mut_;
  std::vector<std::weak_ptr<Request>> obs_;
};

Inventory inventory;

Request::Request() {}

void Request::print() {
  std::unique_lock lock(mut_);
  char buf[] = "Request\n";
  write(STDOUT_FILENO, buf, sizeof(buf));
}

void Request::register1() {
  std::unique_lock lock(mut_);
  inventory.register1(weak_from_this());
}

Request::~Request() {
  std::unique_lock lock(mut_);
  inventory.remove(this);
}

int main(int argc, char const* argv[]) {
  auto e = std::make_shared<Request>();
  e->register1();
  std::thread t([&]() {
    ssleep(2);
    inventory.printAll();
  });

  e->~Request();
  t.join();
  return 0;
}
