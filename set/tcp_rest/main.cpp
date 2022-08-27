#include <arpa/inet.h>
#include <errno.h>
#include <error.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>

int main(int argc, char const *argv[]) {
  auto fd = socket(AF_INET, SOCK_STREAM, 0);

  if (fd < 0) {
    printf("socket error %s", strerror(errno));
  }

  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = inet_addr("127.0.0.1");
  addr.sin_port = htons(1234);
  if (auto e = connect(fd, (struct sockaddr *)&addr, sizeof(addr)); e < 0) {
    printf("socket error %s", strerror(errno));
  }

  return 0;
}
