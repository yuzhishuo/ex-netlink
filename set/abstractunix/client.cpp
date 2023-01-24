#include <errno.h>
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

#define BUFFER_SIZE 1024
const char *filename = "uds-tmp";
const char *filename1 = "uds-tmp1";

int main() {
  struct sockaddr_un un;
  int sock_fd;

  un.sun_family = AF_UNIX;
  un.sun_path[0] = '\0';
  strcpy(un.sun_path + 1, filename);
  sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (sock_fd < 0) {
    perror("Request socket failed");
    return -1;
  }

  struct sockaddr_un un1;
  bzero(&un1, sizeof(un1));
  un1.sun_family = AF_UNIX;
  un1.sun_path[0] = '\0';
  strcpy(un1.sun_path + 1, filename1);
  auto size = offsetof(struct sockaddr_un, sun_path) + strlen(filename1) + 1;
  if (auto e = bind(sock_fd, (struct sockaddr *)&un1, size); e < 0) {
    perror("bind fail");
    return -1;
  }

  size = offsetof(struct sockaddr_un, sun_path) + strlen(filename) + 1;
  if (connect(sock_fd, (struct sockaddr *)&un, size) < 0) {
    perror("connect socket failed\n");
    return -1;
  }

  char buffer[BUFFER_SIZE] = {1, 2, 3};
  while (1) {
    send(sock_fd, buffer, BUFFER_SIZE, 0);
    printf("sending\n");
    sleep(10 * 1000);
  }

  close(sock_fd);
  return 0;
}
