#include <atomic>
#include <functional>
#include <iostream>
#include <thread>

using namespace std;

class ZeroEvenOdd {
 private:
  int n;
  atomic<int> flag = 0;

 public:
  ZeroEvenOdd(int n) { this->n = n; }

  // printNumber(x) outputs "x", where x is an integer.
  void zero(function<void(int)> printNumber) {
    for (int i = 1; i <= n; ++i) {
      while (flag != 0) {
        this_thread::yield();
      }
      printNumber(0);
      if (i % 2 == 0) {
        flag = 2;
      } else {
        flag = 1;
      }
    }
  }

  void even(function<void(int)> printNumber) {
    for (int i = 2; i <= n; i += 2) {
      while (flag != 2) {
        this_thread::yield();
      }
      printNumber(i);
      flag = 0;
    }
  }

  void odd(function<void(int)> printNumber) {
    for (int i = 1; i <= n; i += 2) {
      while (flag != 1) {
        this_thread::yield();
      }
      printNumber(i);
      flag = 0;
    }
  }
};

int main(int argc, char const *argv[]) {
  ZeroEvenOdd zeo(10);
  std::jthread y(&ZeroEvenOdd::zero, &zeo, [](int n) { std::cout << n; });
  std::jthread y1(&ZeroEvenOdd::even, &zeo, [](int n) { std::cout << n; });
  std::jthread y2(&ZeroEvenOdd::odd, &zeo, [](int n) { std::cout << n; });
  return 0;
}
