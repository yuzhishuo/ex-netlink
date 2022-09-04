#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#include <thread>

#include "echo.common.h"

int main(int argc, char const *argv[]) {
  if (access(NAMEDPIPENAME, F_OK) != 0) {
    printf("access pip %s error", NAMEDPIPENAME);
  }

  auto pipo_fd = open(NAMEDPIPENAME, O_RDONLY);

  if (pipo_fd < 0) {
    perror("open pipo error");
    exit(-1);
  }

  auto sock_fd = socket(AF_INET, SOCK_STREAM, 0);

  if (sock_fd < 0) {
    perror("create socket error");
    exit(-1);
  }

#ifdef REDIRECT_STDIN
  auto doc_fd = open(ECHOFILEPATH, O_RDONLY);
  dup2(doc_fd, fileno(stdin));
#endif

  struct sockaddr_in addr;
  bzero(&addr, sizeof(addr));
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_family = AF_INET;
  addr.sin_port = htons(3344);
  if (auto e = bind(sock_fd, (struct sockaddr *)&addr, sizeof(addr)); e != 0) {
    perror("bind error");
    exit(-1);
  }

  bzero(&addr, sizeof(addr));
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_family = AF_INET;
  addr.sin_port = htons(1234);
  if (auto e = connect(sock_fd, (struct sockaddr *)&addr, sizeof(addr));
      e != 0) {
    perror("connect error");
    exit(-1);
  }

  int stand_id_flag = 0;
  fd_set set;
  char read_buf[1024] = {0};

  for (;;) {
    if (stand_id_flag == 0) {
      FD_SET(fileno(stdin), &set);
    }
    FD_SET(sock_fd, &set);

    select(10, &set, nullptr, nullptr, 0);

    if (FD_ISSET(fileno(stdin), &set)) {
      bzero(read_buf, sizeof(read_buf));
      if (fgets(read_buf, sizeof(read_buf), stdin) == nullptr) {
        stand_id_flag = 1;
        shutdown(sock_fd, SHUT_WR);
        FD_CLR(fileno(stdin), &set);
        continue;
      }

      if (write(sock_fd, read_buf, strlen(read_buf)) < 0) {
        perror("send error");
        exit(-1);
      }
    }

    if (FD_ISSET(sock_fd, &set)) {
      bzero(read_buf, sizeof(read_buf));
      if (read(sock_fd, read_buf, sizeof(read_buf)) == 0) {
        perror("read error");
        exit(-1);
      }
      fputs(read_buf, stdout);
    }
  }
  return 0;
}