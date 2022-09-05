#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <vector>

#include "echo.common.h"

void sigchild_handle(int) {
  int pid, process_res;
  while (pid = waitpid(-1, &process_res, WNOHANG) > 0) {
    printf("handle subprocess %d\n", pid);
    fflush(stdout);
  }
}

using sighandler_t = void (*)(int);

sighandler_t signal_t(int s, sighandler_t hanler) {
  struct sigaction act;

  sigemptyset(&act.sa_mask);
  act.sa_flags = 0;
  act.sa_handler = hanler;
  if (s == SIGALRM) {
#ifdef SA_INTERRUPT
    act.sa_flags |= SA_INTERRUPT;
#endif
  } else {
#ifdef SA_RESTART
    act.sa_flags |= SA_RESTART;
#endif
  }
  struct sigaction ract;
  bzero(&ract, sizeof(struct sigaction));
  if (sigaction(s, &act, &ract) != 0) {
    return SIG_ERR;
  }
  return ract.sa_handler;
}

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

  signal_t(SIGCHLD, sigchild_handle);
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
  std::vector<struct pollfd> client_fds{1};

  client_fds[0].fd = sock_fd;
  client_fds[0].events = POLLRDNORM;

  for (;;) {
    auto n = poll(client_fds.data(), client_fds.size(), 1000);

    if (n < 0) {
      perror("poll error");
      exit(-1);
    }

    if (n == 0) {
      continue;
    }

    if (client_fds[0].revents & POLLRDNORM) {
      if (auto connected_fd = accept(sock_fd, nullptr, nullptr);
          connected_fd < 0) {
        perror("accept error :");
        exit(-1);
      } else {
        struct pollfd fd;
        fd.fd = connected_fd;
        fd.events = POLLRDNORM | POLLERR;
        client_fds.emplace_back(std::move(fd));
      }
    }

    for (int i = 1; i < client_fds.size(); i++) {
      if (auto connected_fd = client_fds[i].fd; connected_fd == -1) {
        continue;
      } else {
        if (client_fds[i].revents & (POLLRDNORM | POLLERR)) {
          char buf[1024] = {0};
          if (auto n = read(connected_fd, buf, sizeof(buf)); n > 0) {
            send(connected_fd, buf, n, 0);
          } else if (n == 0) {
            close(connected_fd);
            client_fds[i].fd = -1;
          } else if (n < 0) {
            perror("read error");
          }
        }
      }
    }
  }
  return 0;
}
