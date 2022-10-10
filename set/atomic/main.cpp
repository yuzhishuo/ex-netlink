#include <atomic>
#include <functional>
#include <iostream>
#include <thread>

using namespace std;

class Foo {
 public:
  Foo() { cnt.store(1, std::memory_order_seq_cst); }

  void first(function<void()> printFirst) {
    // printFirst() outputs "first". Do not change or remove this line.
    printFirst();
    cnt.store(2, std::memory_order_release);
  }

  void second(function<void()> printSecond) {
    // printSecond() outputs "second". Do not change or remove this line.
    while (cnt.load() != 2) {
      std::this_thread::yield();
    }
    printSecond();
    cnt.store(3);
  }

  void third(function<void()> printThird) {
    // printThird() outputs "third". Do not change or remove this line.
    while (cnt.load() != 3) {
      std::this_thread::yield();
    }
    printThird();
  }

  atomic_int16_t cnt;
};

int main(int argc, char const *argv[]) {
  cout.setf(std::ios::unitbuf);
  Foo f;

  std::jthread y(&Foo::first, &f, []() { std::cout << "first"; });
  std::jthread y1(&Foo::third, &f, []() { std::cout << "third"; });
  std::jthread y2(&Foo::second, &f, []() { std::cout << "second"; });

  return 0;
}
