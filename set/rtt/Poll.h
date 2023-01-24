#pragma once

#ifndef LULUYUZHI_RTT_POLL_H_
#define LULUYUZHI_RTT_POLL_H_
#include <bits/ranges_algo.h>
#include <bits/ranges_util.h>
#include <poll.h>
#include <sys/poll.h>
#include <sys/time.h>

#include <algorithm>
#include <coroutine>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iterator>
#include <memory>
#include <mutex>
#include <queue>
#include <system_error>
#include <type_traits>
#include <unordered_set>
#include <vector>

#include "Fd.h"
#include "IReactor.h"
#include "IUnique.h"
#include "Socket.h"
#include "Sockops.h"

namespace luluyuzhi::net {

class Poll : public IReactor {
 public:
  Poll() {}
  void registerEventFun(std::function<void(const Fd& fd, bool readable,
                                           bool writeable, bool errorable)>
                            handle) {
    event_handler_ = std::move(handle);
  }

  std::error_condition run(uint64_t timestamp) override {
    std::unique_lock lock(fd_mut_);
    auto listend_fds = fds_;
    lock.unlock();

    auto num = ::poll(listend_fds.data(), listend_fds.size(), timestamp);

    if (num < 0) {
      return std::error_condition{errno, std::system_category()};
    }
    timeval outval;
    if (auto e = gettimeofday(&outval, nullptr); e < 0) {
      return std::error_condition{errno, std::system_category()};
    }
    uint64_t t = outval.tv_sec * 1000000 + outval.tv_usec;

    event_handler(num, listend_fds, t);
    return {};
  }

  void add(const tool::IUnique& unique) override {
    auto socket = dynamic_cast<const net::Socket&>(unique);
    std::unique_lock lock(fd_mut_);
    switch (socket.getModel()) {
      case socket_model::LISTEN_SHARED:
        /* code */
        break;
      case socket_model::LISTEN_EXCLUSE:
      case socket_model::NORMAL:
        if (fdtbs_.contains(unique.getId())) return;
        break;
      default:
        break;
    }

    fdtbs_.insert(std::make_pair(unique.getId(), socket.shared_from_this()));

    short event = 0;
    if (socket.readenable()) event |= POLLIN;
    if (socket.writeenable()) event |= POLLOUT;
    if (socket.errorenable()) event |= POLLERR;

    if (auto itr = std::find_if(fds_.begin(), fds_.end(),
                                [&unique](const struct pollfd& p) {
                                  return p.fd == unique.getId();
                                });
        itr != fds_.end()) {
      itr->fd = socket.getId();
      itr->events = event;
    }

    fds_.push_back(pollfd{.fd = unique.getId(), .events = event});
    // std::mismatch
  }
  void remove(const tool::IUnique& unique) override {
    std::unique_lock lock(fd_mut_);
    auto socket = dynamic_cast<const net::Socket&>(unique);

    if (!fdtbs_.contains(socket.getId())) return;
    fdtbs_.erase(socket.getId());
    auto findfd_condition = [&unique](const struct pollfd& pollfd) -> bool {
      return unique.getId() == pollfd.fd;
    };
    if (auto [first, last] = std::ranges::remove_if(fds_, findfd_condition);
        std::distance(last, first) != 0) {
      fds_.erase(first, last);
    }
  }

  void update(const tool::IUnique& unique) override {
    std::unique_lock lock(fd_mut_);
    auto socket = dynamic_cast<const net::Socket&>(unique);
    std::ranges::find_if(fds_, [&socket](struct pollfd& pollfd) -> bool {
      if (socket.getId() == pollfd.fd) {
        short event = 0;
        if (socket.readenable()) event |= POLLIN;
        if (socket.writeenable()) event |= POLLOUT;
        if (socket.errorenable()) event |= POLLERR;
        pollfd.events = event;
        return true;
      }
      return false;
    });
  }

 private:
  void event_handler(size_t num, const std::vector<struct pollfd>& listend_fds,
                     uint64_t time) {
    for (const auto& f : listend_fds) {
      if (num == 0) break;

      if (f.fd != -1 && fdtbs_.contains(f.fd)) {
        auto& socket = fdtbs_[f.fd];

        if (socket->responsable()) {
          event_handler_(*socket, f.revents & POLLIN, f.revents & POLLOUT,
                         f.revents & POLLERR);
          num--;
        }
      }
    }
  }

 private:
  std::mutex fd_mut_;
  std::vector<struct pollfd> fds_;
  std::unordered_map<decltype(pollfd::fd), std::shared_ptr<net::Socket>> fdtbs_;
  std::function<void(const Fd& fd, bool readable, bool writeable,
                     bool errorable)>
      event_handler_;
};
}  // namespace luluyuzhi::net

#endif