#include "rtt_server.h"

#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <any>
#include <functional>
#include <map>
#include <memory>
#include <vector>

#include "rtt_common.h"
namespace luluyuzhi {

class Connector {
 public:
  Connector(int fd, struct sockaddr_storage addr) : fd_(fd), addr_{addr} {}

 private:
  int fd_;
  struct sockaddr_storage addr_;
  std::any data;
};

class Accept {
 public:
  std::shared_ptr<Connector> create(int fd) {
    struct sockaddr_storage store;
    socklen_t len = sizeof(struct sockaddr_storage);
    int connect_fd;

    if (auto cond =
            net::accept(fd, (struct sockaddr *)&store, &len, connect_fd);
        cond) {
      net::hold_error_condition(cond);
      return nullptr;
    }
    auto connector = std::make_shared<Connector>(connect_fd, store);
    if (!connector) {
      return nullptr;
    }

    if (on_new_connect_) {
      on_new_connect_(connector);
    }
    return connector;
  }

  void setOnNewConnect(
      std::function<void(std::shared_ptr<Connector> connector)> func) {
    on_new_connect_ = std::move(func);
  }

 private:
  std::function<void(std::shared_ptr<Connector> connector)> on_new_connect_;
};

class RttServer final {
 public:
  RttServer() {
    net::createSocket(sock_fd_);

    fds.push_back(
        pollfd{.fd = sock_fd_, .events = POLLIN | POLLERR, .revents = 0});
  }

  int startDomain() & {
    net::setSocketNotBlock(sock_fd_);
    net::setAddressReuse(sock_fd_);
    struct sockaddr_in peer_addr;
    peer_addr.sin_family = AF_INET;
    peer_addr.sin_port = htons(9966);
    if (inet_net_pton(AF_INET, "127.0.0.1", (void *)&peer_addr.sin_addr,
                      sizeof(peer_addr.sin_addr)) < 0) {
      perror("ip is seted fail");
      return -1;
    }

    if (bind(sock_fd_, (struct sockaddr *)&peer_addr, sizeof(peer_addr)) < 0) {
      perror("bind fail");
      return -1;
    }

    net::listen(sock_fd_, 10);

    for (;;) {
      auto fd_size = poll(fds.data(), fds.size(), -1);
      if (fd_size < 0) {
        perror("poll fail");
        return -1;
      }

      if (auto fd = fds[0].fd; fds[0].revents & POLLIN) {
        fd_size--;
        auto connected_fd = accept(fd, nullptr, nullptr);

        if (connected_fd < 0) {
          if (connected_fd != EAGAIN) {
            perror("accept error");
            return -1;
          }
        } else {
          if (auto flag = fcntl(connected_fd, F_GETFL, 0); flag >= 0) {
            if (flag | O_NONBLOCK) {
              printf("connect fd inhert listen sock noblock attribute\n");
            } else {
              printf("connect fd can't inhert listen sock noblock attribute\n");
            }
          }
          onNewConnect(connected_fd);
        }
      }

      for (size_t i = 1; i < fds.size(); i++) {
        if (fds[i].fd == -1) continue;
        if (fd_size <= 0) break;

        if (fds[i].revents & POLLERR || fds[i].revents & POLLIN ||
            fds[i].revents & POLLOUT) {
          fd_size--;
        } else {
          continue;
        }

        auto connected_fd = fds[i].fd;
        if (fds[i].revents & POLLIN) {
          uint8_t buff[1024];
          auto rs = read(connected_fd, &buff, sizeof(buff));

          if (rs <= 0) {
            perror("read fail");
            if (rs != EAGAIN) {
              shutdown(connected_fd, SHUT_RDWR);
              dispose(i);
              continue;
            }
          }

          translations_[connected_fd].rbuffer.append(buff, rs);
          // ????????????????????????????????????
          if (translations_[connected_fd].rbuffer.readed_size() <
              sizeof(rtt_package)) {
            fds[i].events |= POLLIN;
            continue;
          }
          struct timeval val;
          gettimeofday(&val, nullptr);
          // copy
          rtt_package pkg{};

          pkg.c_send_ts = translations_[connected_fd].rbuffer.peek<uint64_t>();
          translations_[connected_fd].rbuffer.skip(sizeof(uint64_t));
          pkg.s_recv_ts = translations_[connected_fd].rbuffer.peek<uint64_t>();
          translations_[connected_fd].rbuffer.skip(sizeof(uint64_t));
          pkg.s_send_ts = translations_[connected_fd].rbuffer.peek<uint64_t>(),
          translations_[connected_fd].rbuffer.skip(sizeof(uint64_t));

          pkg.c_send_ts = luluyuzhi::host2network(pkg.c_send_ts);
          pkg.s_recv_ts = luluyuzhi::host2network(timeval2ui64(val));
          gettimeofday(&val, nullptr);
          pkg.s_send_ts = luluyuzhi::host2network(timeval2ui64(val));
          auto ws = write(connected_fd, reinterpret_cast<void *>(&pkg),
                          sizeof(rtt_package));
          if (ws <= 0) {
            perror("write fail");
            if (errno == EAGAIN) {
              translations_[connected_fd].wbuffer.append(
                  reinterpret_cast<uint8_t *>(&pkg), sizeof(rtt_package) - ws);
              fds[i].events |= POLLOUT;
              fds[i].events &= ~POLLIN;
            } else {
              assert(0);
            }
          }

          if (ws < sizeof(rtt_package)) {
            uint8_t *data;
            data = reinterpret_cast<uint8_t *>(&pkg) + ws;
            translations_[connected_fd].wbuffer.append(
                data, sizeof(rtt_package) - ws);
            fds[i].events |= POLLOUT;
            fds[i].events &= ~POLLIN;
          }
        }

        if (fds[i].revents & POLLOUT) {
          printf("connected fd [%d] gennerates POLLOUT event.\n", connected_fd);
          // ????????????????????????????????????
          auto &connect = translations_[connected_fd];
          auto &wbuff = connect.wbuffer;

          auto wr_num = write(connected_fd, wbuff.peek(), wbuff.readed_size());

          if (wr_num < 0) {
            if (errno != EWOULDBLOCK) {
              shutdown(connected_fd, SHUT_RDWR);
              dispose(i);
            } else {
              fds[i].events |= POLLOUT;
              fds[i].events &= ~POLLIN;
            }
          } else {
            if (wr_num == wbuff.readed_size()) {
              shutdown(connected_fd, SHUT_RDWR);
              dispose(i);
            } else {
              wbuff.skip(wr_num);
              fds[i].events |= POLLOUT;
              fds[i].events &= ~POLLIN;
            }
          }
        }

        if (fds[i].revents & POLLERR) {
          // ???????????????
          int e;
          socklen_t len;
          getsockopt(connected_fd, SOL_SOCKET, SO_ERROR, &e, &len);
          printf("connected fd [%d] generates POLLERR event, that is %s",
                 connected_fd, strerror(e));
          if (e == EINTR) {
            continue;
          }
          dispose(i);
          return -1;
        }
      }
    }
    return 0;
  }

  void dispose(int i) {
    auto fd = fds[i].fd;
    bzero(&fds[i].events, sizeof(fds[i].events));
    translations_.erase(fd);
    fds[i].fd = -1;
  }

  void onNewConnect(int connected_fd) {
    auto fd_itr = std::find_if(fds.begin(), fds.end(),
                               [](const auto &pfd) { return pfd.fd == -1; });
    if (fd_itr != fds.end()) {
      fd_itr->fd = connected_fd;
      fd_itr->events = POLLIN | POLLERR;
    } else {
      fds.push_back(
          pollfd{.fd = connected_fd, .events = POLLIN | POLLERR, .revents = 0});
    }

    assert(!translations_.contains(connected_fd));
    Translation transaction;
    translation_init(transaction, connected_fd);
    translations_.emplace(connected_fd, std::move(transaction));
  }

 private:
  int sock_fd_;
  std::vector<struct pollfd> fds;
  std::map<int, Translation, std::less<>> translations_;
};

}  // namespace luluyuzhi

int main(int argc, char const *argv[]) {
  return std::make_unique<luluyuzhi::RttServer>()->startDomain();
}
