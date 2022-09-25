#include <arpa/inet.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <thread>
#include <vector>

void send_fd(int sock_fd, int send_fd);
int recv_fd(const int sock_fd);

int parent_process(int fd) {
  std::vector<struct pollfd> pollfds;

  int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (sock_fd < 0) {
    perror("main process socket");
  }
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(9901);

  if (auto e = bind(sock_fd, (struct sockaddr *)&addr, sizeof(addr)); e < 0) {
    perror("main process bind");
  }

  listen(sock_fd, 30);

  int async_fd = -1;

  pollfds.emplace_back(pollfd{.fd = sock_fd, .events = POLLIN});
  pollfds.emplace_back(pollfd{.fd = fd, .events = POLLIN});
  while (true) {
    auto answer_num = poll(pollfds.data(), pollfds.size(), -1);

    if (answer_num < 0) {
      perror("main process error");
      continue;
    }

    if (pollfds[0].revents & POLLIN) {
      if (async_fd = accept(pollfds[0].fd, NULL, NULL); async_fd < 0) {
        perror("main process accept");
      }

      pollfds[1].events = POLLOUT | POLLIN;
    }

    if (pollfds[1].revents & POLLIN) {
    }

    if (pollfds[1].revents & POLLOUT) {
      if (async_fd != -1) {
        send_fd(fd, async_fd);
        pollfds[1].events = POLLIN;
      }

      async_fd = -1;
    }
  }
}

int child_process(int fd) {
  std::vector<struct pollfd> pollfds;

  pollfds.emplace_back(pollfd{.fd = fd, .events = POLLIN});

  while (true) {
    auto answer_num = poll(pollfds.data(), pollfds.size(), -1);
    if (answer_num < 0) {
      perror("child process poll");
    }
    if (pollfds[0].revents & POLLIN) {
      auto connecttion_fd = recv_fd(pollfds[0].fd);
      pollfds.emplace_back(pollfd{.fd = connecttion_fd, .events = POLLIN});

      printf("subprocess new connect, fd %d\n", pollfds.back().fd);
      answer_num--;
    }

    for (int i = 1; i < pollfds.size(); i++) {
      if (answer_num == 0) break;

      if (pollfds[i].fd != -1 && pollfds[i].revents & (POLLIN | POLLOUT)) {
        if (pollfds[i].revents & POLLIN) {
          char buf[2048] = {0};
          auto read_number = read(pollfds[i].fd, buf, sizeof(buf));
          if (read_number < 0) {
            if (errno != EINTR) {
              perror("subprocess read error");
              close(pollfds[i].fd);
              pollfds[i].fd = -1;
              pollfds[i].revents = 0;
              continue;
            }
          }

          if (read_number == 0) {
            close(pollfds[i].fd);
            pollfds[i].fd = -1;
            pollfds[i].revents = 0;
            continue;
          }
          printf("new message: %s\n", buf);

          pollfds[i].events = POLLOUT;
        }
        if (pollfds[i].revents & POLLOUT) {
          printf("subprocess out, fd %d\n", pollfds[i].fd);
          char response[] =
              R"(HTTP/1.1	200	OK
Content-Type: text/html;charset=utf-8
Content-length: 77

<html>
  <head>
  </head>
  <body>
    <h1>网页文字</h1>
  </body>
</html>)";

          write(pollfds[i].fd, response, sizeof(response));
          close(pollfds[i].fd);
          pollfds[i].fd = -1;
        }

        answer_num--;
      }
    }
  }
  return 0;
}

int main(int argc, char const *argv[]) {
  int fd[2];
  socketpair(AF_LOCAL, SOCK_STREAM, 0, fd);

  if (auto child_pid = fork(); child_pid == 0) {
    close(fd[1]);
    exit(parent_process(fd[0]));
  }

  close(fd[0]);
  exit(child_process(fd[1]));
}

void send_fd(int sock_fd, int send_fd) {
  int ret;
  struct msghdr msg;
  struct cmsghdr *p_cmsg;
  struct iovec vec;
  char cmsgbuf[CMSG_SPACE(sizeof(send_fd))];
  int *p_fds;
  char sendchar = 0;
  msg.msg_control = cmsgbuf;
  msg.msg_controllen = sizeof(cmsgbuf);
  p_cmsg = CMSG_FIRSTHDR(&msg);
  p_cmsg->cmsg_level = SOL_SOCKET;
  p_cmsg->cmsg_type = SCM_RIGHTS;
  p_cmsg->cmsg_len = CMSG_LEN(sizeof(send_fd));
  p_fds = (int *)CMSG_DATA(p_cmsg);
  *p_fds = send_fd;  // 通过传递辅助数据的方式传递文件描述符

  msg.msg_name = NULL;
  msg.msg_namelen = 0;
  msg.msg_iov = &vec;
  msg.msg_iovlen = 1;  //主要目的不是传递数据，故只传1个字符
  msg.msg_flags = 0;

  vec.iov_base = &sendchar;
  vec.iov_len = sizeof(sendchar);
  ret = sendmsg(sock_fd, &msg, 0);
  if (ret != 1) exit(0);
}

int recv_fd(const int sock_fd) {
  int ret;
  struct msghdr msg;
  char recvchar;
  struct iovec vec;
  int recv_fd;
  char cmsgbuf[CMSG_SPACE(sizeof(recv_fd))];
  struct cmsghdr *p_cmsg;
  int *p_fd;
  vec.iov_base = &recvchar;
  vec.iov_len = sizeof(recvchar);
  msg.msg_name = NULL;
  msg.msg_namelen = 0;
  msg.msg_iov = &vec;
  msg.msg_iovlen = 1;
  msg.msg_control = cmsgbuf;
  msg.msg_controllen = sizeof(cmsgbuf);
  msg.msg_flags = 0;

  p_fd = (int *)CMSG_DATA(CMSG_FIRSTHDR(&msg));
  *p_fd = -1;
  ret = recvmsg(sock_fd, &msg, 0);
  if (ret != 1) exit(0);

  p_cmsg = CMSG_FIRSTHDR(&msg);
  if (p_cmsg == NULL) exit(0);
  p_fd = (int *)CMSG_DATA(p_cmsg);
  recv_fd = *p_fd;
  if (recv_fd == -1) exit(0);

  return recv_fd;
}