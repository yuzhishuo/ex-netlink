#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "echo.common.h"

int create_fifo() {
  if (access(NAMEDPIPENAME, F_OK) != 0) {
    if (auto e = mkfifo(NAMEDPIPENAME, 0777); e != 0) {
      printf("make fifo fail: %s \n", strerror(errno));
      exit(-1);
    }
  }

  auto open_mode = O_WRONLY;
  int fifo_id = open(NAMEDPIPENAME, open_mode);
  if (fifo_id == -1) {
    printf("open named fifo fail : %s \n", strerror(errno));
    exit(-1);
  }
  return fifo_id;
}

int main(int argc, char const *argv[]) {
  sigset_t sigs;
  sigemptyset(&sigs);
  sigaddset(&sigs, SIGHUP);
  sigaddset(&sigs, SIGPIPE);
  sigaddset(&sigs, SIGURG);

  sigprocmask(SIG_BLOCK, &sigs, nullptr);

  auto fifo_fd = create_fifo();
  int sock_fd = socket(AF_INET, SOCK_STREAM, 0);

  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(1234);
  addr.sin_addr.s_addr = inet_addr("127.0.0.1");

  if (auto e = bind(sock_fd, (struct sockaddr *)&addr, sizeof(addr)); e != 0) {
    perror("bind error :");
    exit(-1);
  }

  if (auto e = listen(sock_fd, 100); e != 0) {
    perror("listen error :");
    exit(-1);
  }

  int notify[1] = {1};
  if (auto e = write(fifo_fd, notify, sizeof(notify)); e < 0) {
    perror("write fifo error :");
    exit(-1);
  }

  while (1) {
    if (auto connected_fd = accept(sock_fd, nullptr, nullptr);
        connected_fd < 0) {
      perror("accept error :");
      exit(-1);
    } else {
      if (auto pid = fork(); pid == 0) {
        int n = 0;
        char buf[1024] = {0};
      again:
        while (n = recv(connected_fd, buf, sizeof(buf), 0)) {
          send(connected_fd, buf, n, 0);
        }

        if (n < 0 && errno == EINTR) {
          goto again;
        } else {
          perror("recv error :");
          exit(-1);
        }
      }

      close(connected_fd);
    }
  }

  return 0;
}
