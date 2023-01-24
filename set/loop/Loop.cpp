#include "Loop.h"

#include <assert.h>
#include <string.h>
#include <strings.h>
#include <time.h>

#include "IHandle.h"
#include "Thread.h"

static thread_local uint64_t outtime = 0;

namespace luluyuzhi::tool {
static void push_runnable(std::vector<std::function<void()>>& runnables,
                          std::function<void()> runnable,
                          std::mutex& mut) noexcept;

static uint64_t get_current_timestamp() noexcept {
  struct timeval val;
  bzero(&val, sizeof(struct timeval));
  gettimeofday(&val, NULL);
  return val.tv_sec * 1'000'000 + val.tv_usec;
}

Loop::Loop() : is_quit_(true){};
Loop::~Loop() { assert(is_quit_ == true); }

void Loop::run() {
  is_quit_ = false;
  while (true) {
    if (is_quit_) break;
    handle_runnable(idle_, idle_mut_);
    handle_io_source(get_outtime());
    handle_timer(timer_, get_current_timestamp());
    handle_runnable(runable_, run_mut_);
  }
}

void Loop::handle_io_source(uint64_t outtime) {
  if (io_handle_) io_handle_->run(outtime);
}

uint64_t Loop::get_outtime() const {
  uint64_t t = get_current_timestamp();
  return t > outtime ? t - outtime : 0;
}

void Loop::pushIdle(std::function<void()> runnable) noexcept {
  push_runnable(runable_, std::move(runnable), idle_mut_);
}
void Loop::pushRunable(std::function<void()> runnable) noexcept {
  push_runnable(idle_, std::move(runnable), run_mut_);
}

void Loop::quit() noexcept { is_quit_ = true; }

void Loop::runAt(uint64_t timestamp, std::function<void()> func) noexcept {
  std::unique_lock l{time_mut_};
  timer_.emplace(timestamp, func);
}

void Loop::registerIoHandle(std::shared_ptr<net::IHandle> handle) noexcept {
  io_handle_ = handle;
}
void Loop::handle_timer(
    std::priority_queue<timer, std::vector<timer>, std::greater<timer>>& timer,
    uint64_t timestamp) {
  uint64_t t = outtime;
  std::unique_lock l{time_mut_};
  while (!timer.empty()) {
    const auto& timeout_element = timer.top();
    if (t = timeout_element.timestamp; t > timestamp) break;
    timeout_element.runnable();
    timer.pop();  // timeout_element invail
  }
  outtime = t;
}

void push_runnable(std::vector<std::function<void()>>& runnables,
                   std::function<void()> runnable, std::mutex& mut) noexcept {
  std::unique_lock l{mut};
  runnables.emplace_back(runnable);
}

void inline Loop::handle_runnable(std::vector<std::function<void()>>& runnables,
                                  std::mutex& mut) {
  std::remove_reference_t<decltype(runnables)> funcs;
  {
    std::unique_lock l{mut};
    funcs.swap(runnables);
  }
  std::ranges::for_each(std::as_const(funcs), [](auto& f) { f(); });
}

}  // namespace luluyuzhi::tool