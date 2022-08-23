#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int main(int argc, char const *argv[]) {
  if (argc < 2) {
    char used[52] = {};
    snprintf(used, sizeof(used),
             "Usage variables number must greater then %d numbers", argc);
    printf("%s\n", used);
  }

  if (int fd = socket(AF_INET, SOCK_STREAM, 0); fd < 0) {
    printf("socket error: %s\n", strerror(errno));
  } else {
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(argv[1]));
    addr.sin_addr.s_addr = inet_addr(argv[2]);

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
      printf("connect error: %s\n", strerror(errno));
    } else {
      printf("connected\n");
      char buf[256] = {0};
      while (read(fd, buf, sizeof(buf)) > 0) {
        printf("%s\n", buf);
      }
    }
  }

  return 0;
}
