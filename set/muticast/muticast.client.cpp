#include <stdio.h>
#include <sys/types.h> /* See NOTES
*/
#include <arpa/inet.h>
#include <netinet/in.h>
#include <strings.h>
#include <sys/socket.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (-1 == sockfd) {
    perror("socket");
    return -1;
  }

  struct ip_mreq mreq;
  bzero(&mreq, sizeof(mreq));
  mreq.imr_multiaddr.s_addr = inet_addr("224.224.224.224");     // 组播ip
  mreq.imr_interface.s_addr = inet_addr("192.168.1.21");  // 主机ip

  // 加入多播组
  if (-1 ==
      setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq))) {
    perror("setsockopt");
    return -1;
  }

  struct sockaddr_in addr;
  bzero(&addr, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(9999);                      // 接收方的端口号
  addr.sin_addr.s_addr = inet_addr("224.224.224.224");  // 广播地址

  int ret = bind(sockfd, (struct sockaddr *)&addr, sizeof(addr));
  if (ret == -1) {
    perror("bind");
    return -1;
  }
  char buf[1024] = {0};
  while (1) {
    if (-1 == recvfrom(sockfd, buf, sizeof(buf), 0, NULL, NULL)) {
      perror("recvfrom");
      break;
    }
    puts(buf);
  }
  close(sockfd);
  return 0;
}