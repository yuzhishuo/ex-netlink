#include <chrono>
#include <iostream>

#include "thread_pool.h"
using namespace std::chrono;
int main(int argc, char const *argv[]) {
  thread_pool tp;
  tp.push([]() { throw "132"; });
  for (auto i = 0; i < 10; i++)
    tp.push([=]() {
      std::this_thread::sleep_for(5s);
      std::cout << i << std::endl;
    });

  tp.stop();
  return 0;
}
