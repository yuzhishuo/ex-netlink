#include <arpa/inet.h>
#include <assert.h>
#include <endian.h>
#include <error.h>
#include <netinet/tcp.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <string>
#include <utility>

#include "rtt_common.h"

class RttClient {
 public:
  RttClient() {
    sock_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    assert(sock_fd_ > 0);
  }

  std::pair<uint64_t, uint64_t> Connect(const std::string& ip, uint16_t port) {
    if (sock_fd_ < 0) {
      perror("create socket fail");
      return {-1, -1};
    }

    int fl = 1;
    setsockopt(sock_fd_, IPPROTO_TCP, TCP_NODELAY, &fl, sizeof(int));

    struct sockaddr_in peer_addr;
    peer_addr.sin_family = AF_INET;
    peer_addr.sin_port = htons(port);
    if (inet_net_pton(AF_INET, ip.data(), (void*)&peer_addr.sin_addr,
                      sizeof(peer_addr.sin_addr)) < 0) {
      perror("ip is seted fail");
      return {-1, -1};
    }
    if (connect(sock_fd_, (struct sockaddr*)&peer_addr, sizeof(peer_addr)) <
        0) {
      perror("connect fail");
      return {-1, -1};
    }

    struct timeval val;

    gettimeofday(&val, nullptr);

    rtt_package pck = {.c_send_ts = timeval2ui64(val)};

    auto write_size = write(sock_fd_, (void*)&pck, sizeof(rtt_package));
    assert(write_size == sizeof(rtt_package));
    bzero(&pck, sizeof(rtt_package));
    auto read_size = read(sock_fd_, &pck, sizeof(rtt_package));
    assert(read_size == sizeof(rtt_package));

    bzero(&val, sizeof(struct timeval));
    gettimeofday(&val, nullptr);
    uint64_t recv_time = timeval2ui64(val);

    return {
        ((recv_time - pck.c_send_ts) - (pck.s_send_ts - pck.s_recv_ts)),
        ((recv_time + pck.c_send_ts) - (pck.s_send_ts + pck.s_recv_ts)) / 2};
  }

 private:
  int sock_fd_;
};

int main(int argc, char const* argv[]) {
  auto rtt = RttClient{}.Connect("127.0.0.1", 9966);
  printf("rtt is %lu, time diff is %lu \n", rtt.first, rtt.second);
  return 0;
}
