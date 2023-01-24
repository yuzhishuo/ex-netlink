#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h> /* See NOTES */
#include <unistd.h>

int main(int argc, char *argv[]) {
  int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (-1 == sockfd) {
    perror("socket");
    return -1;
  }

  struct sockaddr_in addr;
  bzero(&addr, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(9999);
  addr.sin_addr.s_addr = inet_addr("224.224.224.224");

  char buf[1024] = {};
  while (1) {
    if (-1 == sendto(sockfd, buf, sizeof(buf), 0, (struct sockaddr *)&addr,
                     sizeof(addr))) {
      perror("sendto");
      break;
    }
  }
  close(sockfd);
  return 0;
}