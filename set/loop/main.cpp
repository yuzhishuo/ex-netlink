#include <iterator>

#include "Loop.h"
#include "Thread.h"
int main(int argc, char const *argv[]) {
  using namespace luluyuzhi::tool;

  Loop loop;

  Thread thread{[&loop](Thread *, std::any) { loop.run(); }, nullptr, "thread"};

  auto name = thread.getThreadName();
  loop.pushRunable([]() { std::cout << "hello word" << std::endl; });
  loop.pushRunable([&]() { loop.quit(); });
  return 0;
}
