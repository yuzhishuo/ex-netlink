#include <asm-generic/errno-base.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#define MAX_CONNECT_NUM 10

const char *filename = "uds-tmp";

int main() {
  int fd;
  struct sockaddr_un un;
  fd = socket(AF_UNIX, SOCK_STREAM, 0);

  if (fd < 0) {
    perror("Request socket failed!");
    return -1;
  }
  bzero(&un, sizeof(struct sockaddr_un));
  un.sun_family = AF_UNIX;
  un.sun_path[0] = '\0';
  strcpy(un.sun_path + 1, filename);
  auto size = offsetof(struct sockaddr_un, sun_path) + strlen(filename) + 1;
  if (bind(fd, (struct sockaddr *)&un, size) < 0) {
    perror("bind failed!");
    return -1;
  }

  if (listen(fd, MAX_CONNECT_NUM) < 0) {
    perror("listen failed!\n");
    return -1;
  }

  while (1) {
    struct sockaddr_un client_addr;
    bzero(&client_addr, sizeof(struct sockaddr_un));
    socklen_t len = sizeof(client_addr);
    auto new_fd = accept(fd, (struct sockaddr *)&client_addr, &len);

    auto e = errno;
    if (new_fd < 0 && e == EAGAIN) {
      continue;
    }
    if (new_fd > 0) {
      perror("connect sucess");
    }
  }
  close(fd);
  return -1;
}
