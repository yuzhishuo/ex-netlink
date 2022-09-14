#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <string.h>
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

  do {
    auto baudu_info = gethostbyname("www.baidu.com");
    if (!baudu_info) {
      puts("Get IP address error!");
      break;
    }

    for (int i = 0; baudu_info->h_aliases[i]; i++) {
      printf("Aliases %d: %s\n", i + 1, baudu_info->h_aliases[i]);
    }

    printf("Address type: %s\n",
           (baudu_info->h_addrtype == AF_INET) ? "AF_INET" : "AF_INET6");

    for (int i = 0; baudu_info->h_addr_list[i]; i++) {
      printf("IP addr %d: %s\n", i + 1,
             inet_ntoa(*(struct in_addr *)baudu_info->h_addr_list[i]));
    }
  } while (false);

  printf("---------------\n");
#if _POSIX_C_SOURCE >= 1 || _XOPEN_SOURCE || _POSIX_SOURCE
  do {
    struct addrinfo hints, *rp, *result;

    bzero(&hints, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;                /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_DGRAM;             /* Datagram socket */
    hints.ai_flags = AI_PASSIVE | AI_CANONNAME; /* For wildcard IP address */
    hints.ai_protocol = 0;                      /* Any protocol */
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    if (auto e = getaddrinfo("45.113.192.101", "80", &hints, &result); e != 0) {
      printf("getaddrinfo error %s", gai_strerror(e));
      break;
    }

    for (rp = result; rp != NULL; rp = rp->ai_next) {
      printf("address type: %s\n",
             (rp->ai_family == AF_INET) ? "AF_INET" : "AF_INET6");

      char buf[1024] = {0};
      if (inet_ntop(rp->ai_family, (void *)rp->ai_addr, buf, sizeof(buf)) ==
          nullptr) {
        printf("inet_ntop error");
        break;
      }
      printf("address %s \n", buf);
      printf("rp canonname: %s \n", rp->ai_canonname);
    }

    freeaddrinfo(result);

  } while (false);
#endif
  return 0;
}
