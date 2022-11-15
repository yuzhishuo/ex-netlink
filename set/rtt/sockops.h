#pragma once
#ifndef LULUYUZHI_RTT_SOCKOPS_H
#define LULUYUZHI_RTT_SOCKOPS_H
#include <error.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdint.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

#include <algorithm>
#include <string>
#include <system_error>

namespace luluyuzhi {

class net_system_category : public std::error_category {
 private:
  std::string message(int e) const override {
    std::string res(size_t(128), char(0));
    strerror_r(e, res.data(), res.size());
    return res;
  }

 public:
  virtual const char* name() const noexcept { return "net_system_category"; }

  static std::error_category& constructor() {
    static net_system_category category{};
    return category;
  }
};
class custom_category : public std::error_category {
 private:
  std::string message(int e) const override {
    std::string res(size_t(128), char(0));
    strerror_r(e, res.data(), res.size());
    return res;
  }

 public:
  virtual const char* name() const noexcept { return "custom_category"; }

  static std::error_category& constructor() {
    static custom_category category{};
    return category;
  }
};

}  // namespace luluyuzhi

namespace luluyuzhi::net {
std::error_condition setSocketNotDelay(int fd) {
  int fl = 1;
  if (auto e = setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &fl, sizeof(int));
      e < 0) {
    return {errno, std::system_category()};
  }
  return {};
}

std::error_condition setSocketNotBlock(int fd) {
  if (auto flag = fcntl(fd, F_GETFL, 0); flag >= 0) {
    if (flag | O_NONBLOCK) return {};
    flag |= O_NONBLOCK;
    if (auto e = fcntl(fd, F_SETFL, flag); e < 0) {
      goto error_process;
    }
    return {};
  }

error_process:

  return {errno, std::system_category()};
}

std::error_condition setAddressReuse(int fd) {
  int fl = 1;
  if (auto e = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &fl, sizeof(int));
      e < 0) {
    return {errno, std::system_category()};
  }

  return {};
}

std::error_condition createSocket(int& fd) {
  if (fd = socket(AF_INET, SOCK_STREAM, 0); fd > 0) {
    return {};
  }
  return {errno, std::system_category()};
}

std::error_condition connect(int fd, struct ::sockaddr_in* peer_addr) {
  if (::connect(fd, (struct ::sockaddr*)peer_addr,
                sizeof(struct ::sockaddr_in)) < 0) {
    return {errno, std::system_category()};
  }

  return {};
}

std::error_condition connectAsync(int fd, struct ::sockaddr_in* peer_addr,
                                  long int timeout) {
  setSocketNotBlock(fd);
  fd_set wfds;
  FD_ZERO(&wfds);
  FD_SET(fd, &wfds);
  timeval tv = {timeout, 0};

  while (true) {
    auto re_code = ::connect(fd, (struct ::sockaddr*)peer_addr,
                             sizeof(struct ::sockaddr_in));
    if (re_code == 0) {
      return {};
    }
    auto save_errno = errno;
    if (re_code == -1 && save_errno == EAGAIN) {
      continue;
    }
    if (re_code == -1 && save_errno == EINPROGRESS) {
      break;
    }
    close(fd);
    return {errno, std::system_category()};
  }

  if (auto ret = select(fd + 1, nullptr, &wfds, nullptr, &tv); ret == 1) {
    if (FD_ISSET(fd, &wfds)) {
      int error;
      socklen_t error_len = sizeof(int);
      ret = getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &error_len);
      if (ret == -1 || error != 0) {
        close(fd);
        return {errno, std::system_category()};
      }
      return {};
    }
    close(fd);
    return {errno, std::system_category()};
  }
  close(fd);
  return std::make_error_condition(std::errc::timed_out);
}

std::error_condition accept(int fd, struct ::sockaddr* addr, ::socklen_t* len,
                            int& connect_fd) {
  if (connect_fd = ::accept(fd, addr, len); connect_fd < 0) {
    return {errno, std::system_category()};
  }
  return {};
}

std::error_condition listen(int fd, int backlog) {
  if (auto error = ::listen(fd, backlog); error < 0) {
    return {errno, std::system_category()};
  }
  return {};
}
static std::function<void(const std::string& message)>
    hold_error_condition_out =
        [](const std::string& message) { std::printf("%s", message.c_str()); };

void hold_error_condition(const std::error_condition& cond) {
  if (cond && cond.category() == std::system_category()) {
    hold_error_condition_out ? hold_error_condition_out(cond.message())
                             : void(0);
  }
}

}  // namespace luluyuzhi::net

#endif  // LULUYUZHI_RTT_SOCKOPS_H