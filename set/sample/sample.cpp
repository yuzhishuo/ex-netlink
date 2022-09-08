#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <sys/socket.h>

int main(int argc, char const *argv[]) {
  auto so = socket(AF_INET, SOCK_STREAM, 0);

  socklen_t socklen = 0;
  {
    int max_seg_size = 0;
    socklen = sizeof(int);
    getsockopt(so, IPPROTO_TCP, TCP_MAXSEG, &max_seg_size, &socklen);

    printf("max segment size %d\n", max_seg_size);  // 536
  }

  {
    int recv_buf_size = 0;
    socklen = sizeof(int);
    getsockopt(so, SOL_SOCKET, SO_RCVBUF, &recv_buf_size, &socklen);
    printf("recv buffer size %d\n", recv_buf_size);  // 131072
  }

  {
    int send_buf_size = 0;
    socklen = sizeof(int);
    getsockopt(so, SOL_SOCKET, SO_SNDBUF, &send_buf_size, &socklen);
    printf("send buffer size %d\n", send_buf_size);  // 16384
  }

  {
    int recv_buf_size = 0;
    socklen = sizeof(int);
    getsockopt(so, SOL_SOCKET, SO_RCVLOWAT, &recv_buf_size, &socklen);
    printf("recv buffer LOWAT size %d\n", recv_buf_size);  // 1
  }

  {
    int send_buf_size = 0;
    socklen = sizeof(int);
    getsockopt(so, SOL_SOCKET, SO_SNDLOWAT, &send_buf_size, &socklen);
    printf("send buffer LOWAT size %d\n", send_buf_size);  // 1
  }

  return 0;
}
