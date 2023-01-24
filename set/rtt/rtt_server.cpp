#include "rtt_server.h"

#include <ThreadPool.h>
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
#include <type_traits>
#include <vector>

#include "Accept.h"
#include "Loop.h"
#include "Poll.h"
#include "Socket.h"
#include "Thread.h"
#include "rtt_common.h"
namespace luluyuzhi {

class RttServer final {
 public:
  RttServer()
      : poll_{std::make_shared<net::Poll>()},
        acceptor_{std::dynamic_pointer_cast<net::IReactor>(poll_),
                  net::socket_model::LISTEN_SHARED} {
    using namespace std::placeholders;
    acceptor_.setOnNewConnect(std::bind(&RttServer::onNewConnect, this, _1));
    loop_.registerIoHandle(std::dynamic_pointer_cast<net::IHandle>(poll_));
    poll_->registerEventFun(
        std::bind(&RttServer::handleEvent, this, _1, _2, _3, _4));
    tool::Thread thread{[this](tool::Thread *, std::any) { loop_.run(); },
                        nullptr, "thread"};
  }

  int start() {
    acceptor_.build("127.0.0.1", 9909);
    return 1;
  }

  int startDomain() & {
    for (;;) {
      auto fd_size = 1;
      if (fd_size < 0) {
        perror("poll fail");
        return -1;
      }

      if (auto fd = fds[0].fd; fds[0].revents & POLLIN) {
        fd_size--;

        auto connector = acceptor_.create();

        if (!connector) {
          // if (connected_fd != EAGAIN) {
          perror("accept error");
          return -1;
          //}
        }

        if (auto flag = fcntl(connector->getRawHandler(), F_GETFL, 0);
            flag >= 0) {
          if (flag | O_NONBLOCK) {
            printf("connect fd inhert listen sock noblock attribute\n");
          } else {
            printf("connect fd can't inhert listen sock noblock attribute\n");
          }
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

          auto translation =
              translations_[connected_fd]->getData<Translation>();

          translation->rbuffer.append(buff, rs);
          // 此时就已经进入业务逻辑了
          if (translation->rbuffer.readed_size() < sizeof(rtt_package)) {
            fds[i].events |= POLLIN;
            continue;
          }
          struct timeval val;
          gettimeofday(&val, nullptr);
          // copy
          rtt_package pkg{};

          pkg.c_send_ts = translation->rbuffer.peek<uint64_t>();
          translation->rbuffer.skip(sizeof(uint64_t));
          pkg.s_recv_ts = translation->rbuffer.peek<uint64_t>();
          translation->rbuffer.skip(sizeof(uint64_t));
          pkg.s_send_ts = translation->rbuffer.peek<uint64_t>(),
          translation->rbuffer.skip(sizeof(uint64_t));

          pkg.c_send_ts = luluyuzhi::host2network(pkg.c_send_ts);
          pkg.s_recv_ts = luluyuzhi::host2network(timeval2ui64(val));
          gettimeofday(&val, nullptr);
          pkg.s_send_ts = luluyuzhi::host2network(timeval2ui64(val));
          auto ws = write(connected_fd, reinterpret_cast<void *>(&pkg),
                          sizeof(rtt_package));
          if (ws <= 0) {
            perror("write fail");
            if (errno == EAGAIN) {
              translation->wbuffer.append(reinterpret_cast<uint8_t *>(&pkg),
                                          sizeof(rtt_package) - ws);
              fds[i].events |= POLLOUT;
              fds[i].events &= ~POLLIN;
            } else {
              assert(0);
            }
          }

          if (ws < sizeof(rtt_package)) {
            uint8_t *data;
            data = reinterpret_cast<uint8_t *>(&pkg) + ws;
            translation->wbuffer.append(data, sizeof(rtt_package) - ws);
            fds[i].events |= POLLOUT;
            fds[i].events &= ~POLLIN;
          }
        }

        if (fds[i].revents & POLLOUT) {
          printf("connected fd [%d] gennerates POLLOUT event.\n", connected_fd);
          // 此时就已经进入业务逻辑了
          auto translation =
              translations_[connected_fd]->getData<Translation>();
          auto &wbuff = translation->wbuffer;

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
          // 非业务逻辑
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

  void handleEvent(const net::Fd &fd, bool readable, bool writeable,
                   bool errorable) {}

  void onNewConnect(std::shared_ptr<luluyuzhi::net::Connector> connector) {
    auto connected_fd = connector->getRawHandler();
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
    translation_init(transaction);
    // connector->setData(transaction);
    translations_.emplace(connected_fd, std::move(connector));
  }

  void onReadablity(std::shared_ptr<luluyuzhi::net::Connector> connector,
                    circulation_buffer &buffer, int64_t timestamp) {}

 private:
  std::shared_ptr<net::Poll> poll_;
  tool::Loop loop_;
  tool::ThreadPool pool_;
  std::vector<struct pollfd> fds;
  net::Accept acceptor_;
  std::map<int, std::shared_ptr<luluyuzhi::net::Connector>, std::less<>>
      translations_;
};

}  // namespace luluyuzhi

int main(int argc, char const *argv[]) {
  return std::make_unique<luluyuzhi::RttServer>()->start();
}
