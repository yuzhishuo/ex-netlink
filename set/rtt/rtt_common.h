#ifndef EXNETLINK_RTT_RTTCOMMON_CPP
#define EXNETLINK_RTT_RTTCOMMON_CPP
#include <fcntl.h>
#include <netinet/tcp.h>
#include <stdint.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

static_assert(sizeof(int64_t) == 8, "rtt protocol require int64_t is 8 bytes.");

struct alignas(1) rtt_package {
  uint64_t c_send_ts = 0;
  uint64_t s_recv_ts = 0;
  uint64_t s_send_ts = 0;
};
static_assert(sizeof(rtt_package) == 24);

uint64_t timeval2ui64(struct timeval& val) {
  constexpr static auto unitdiff = 100'0000;
  return val.tv_sec * unitdiff + val.tv_usec;
}

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
}  // namespace luluyuzhi

namespace luluyuzhi::net {
void setSocketNotDelay(int fd) {
  int fl = 1;
  if (auto e = setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &fl, sizeof(int));
      e < 0) {
    throw std::system_error(
        std::error_code{errno, net_system_category::constructor()},
        "setsockopt not delay fail");
  }
}

void setSocketNotBlock(int fd) {
  if (auto flag = fcntl(fd, F_GETFL, 0); flag >= 0) {
    if (flag | O_NONBLOCK) return;
    flag |= O_NONBLOCK;
    if (auto e = fcntl(fd, F_SETFL, flag); e < 0) {
      goto error_process;
    }
    return;
  }

error_process:
  throw std::system_error(
      std::error_code{errno, net_system_category::constructor()},
      "setsockopt not delay fail");
}

void setAddressReuse(int fd) {
  int fl = 1;
  if (auto e = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &fl, sizeof(int));
      e < 0) {
    throw std::system_error(
        std::error_code{errno, net_system_category::constructor()},
        "setsockopt not delay fail");
  }
}

auto createSocket() {
  if (auto fd = socket(AF_INET, SOCK_STREAM, 0); fd > 0) {
    return fd;
  }

  throw std::system_error(
      std::error_code{errno, net_system_category::constructor()},
      "socket create");
}

void connect(int fd, struct ::sockaddr_in* peer_addr) {
  if (::connect(fd, (struct ::sockaddr*)peer_addr,
                sizeof(struct ::sockaddr_in)) < 0) {
    throw std::system_error(
        std::error_code{errno, net_system_category::constructor()},
        "connect fail");  // like perror
  }
}

enum class connect_status : int { normal, timeout, error };

connect_status connectAsync(int fd, struct ::sockaddr_in* peer_addr,
                            long int timeout, std::error_code* code) {
  setSocketNotBlock(fd);

  fd_set wfds;
  FD_ZERO(&wfds);
  FD_SET(fd, &wfds);
  timeval tv = {timeout, 0};

  while (true) {
    auto re_code = ::connect(fd, (struct ::sockaddr*)peer_addr,
                             sizeof(struct ::sockaddr_in));
    if (re_code == 0) {
      return connect_status::normal;
    }
    auto save_errno = errno;
    if (re_code < 0 && save_errno == EAGAIN) {
      continue;
    }
    if (re_code == -1 && save_errno == EINPROGRESS) {
      break;
    }
    close(fd);
    *code =
        std::move(std::error_code{errno, net_system_category::constructor()});
    return connect_status::error;
  }

  if (auto ret = select(fd + 1, nullptr, &wfds, nullptr, &tv); ret == 1) {
    if (FD_ISSET(fd, &wfds)) {
      int error;
      socklen_t error_len = sizeof(int);
      ret = getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &error_len);
      if (ret == -1 || error != 0) {
        close(fd);
        *code = std::move(
            std::error_code{errno, net_system_category::constructor()});
        return connect_status::error;
      }
      return connect_status::normal;
    }
    *code =
        std::move(std::error_code{errno, net_system_category::constructor()});
    return connect_status::error;
  }
  return connect_status::timeout;
}  // namespace luluyuzhi::net

}  // namespace luluyuzhi::net

#endif