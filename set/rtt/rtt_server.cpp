#include "rtt_server.h"

#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <map>
#include <memory>
#include <vector>

#include "rtt_common.h"

class RttServer final {
 public:
  RttServer() {
    sock_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    assert(sock_fd_ > 0);

    fds.push_back(
        pollfd{.fd = sock_fd_, .events = POLLIN | POLLERR, .revents = 0});
  }

  int startDomain() & {
    if (sock_fd_ < 0) {
      perror("create socket fail");
      return -1;
    }

    int fl = 1;
    setsockopt(sock_fd_, IPPROTO_TCP, TCP_NODELAY, &fl, sizeof(int));

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

    listen(sock_fd_, 10);

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
          perror("accept error");
          return -1;
        }
        onNewConnect(connected_fd);
      }

      for (size_t i = 1; i < fds.size(); i++) {
        if (fds[i].fd == -1) continue;
        if (fd_size <= 0) break;

        auto connected_fd = fds[i].fd;
        if (fds[i].revents & POLLIN) {
          rtt_package pkg;

          struct timeval val;
          gettimeofday(&val, nullptr);
          auto rs = read(connected_fd, &pkg, sizeof(rtt_package));

          if (rs == 0) {
            shutdown(connected_fd, SHUT_RDWR);
            dispose(i);
            continue;
          }

          pkg.s_recv_ts = timeval2ui64(val);
          translation_add_data(translations_[connected_fd], pkg);
          fds[i].events |= POLLOUT;
          fds[i].events &= ~POLLIN;
        }

        if (fds[i].revents & POLLOUT) {
          printf("connected fd [%d] gennerates POLLOUT event.\n", connected_fd);
          auto *pkg =
              translation_get_data<rtt_package>(translations_[connected_fd]);
          if (pkg == nullptr) {
            printf("prase user data fail");
          }
          struct timeval val;
          gettimeofday(&val, nullptr);
          pkg->s_send_ts = timeval2ui64(val);
          write(connected_fd, pkg, sizeof(rtt_package));

          shutdown(connected_fd, SHUT_RDWR);

          dispose(i);
        }

        if (fds[i].revents & POLLERR) {
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
        fd_size--;
      }
    }
    return 0;
  }

  void dispose(int i) {
    auto fd = fds[i].fd;
    bzero(&fds[i].events, sizeof(fds[i].events));
    translation_destory(translations_[fd]);
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
    translations_.emplace(connected_fd, transaction);
  }

 private:
  int sock_fd_;
  std::vector<struct pollfd> fds;
  std::map<int, Translation, std::less<>> translations_;
};

int main(int argc, char const *argv[]) {
  return std::make_unique<RttServer>()->startDomain();
}
