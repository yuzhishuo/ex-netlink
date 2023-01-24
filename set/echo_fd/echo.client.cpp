#include <arpa/inet.h>
#include <fcntl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#include <array>
#include <limits>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include "echo.common.h"

#define REDIRECT_STDIN

int echo_client(int ip) {
#ifdef WAIT_WAKEUP
  if (access(NAMEDPIPENAME, F_OK) != 0) {
    printf("access pip %s error", NAMEDPIPENAME);
  }

  auto pipo_fd = open(NAMEDPIPENAME, O_RDONLY);

  if (pipo_fd < 0) {
    perror("open pipo error");
    exit(-1);
  }
#endif

reconnect:

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
  addr.sin_addr.s_addr = ip;
  addr.sin_family = AF_INET;
  addr.sin_port = 0;
  int one = 1;
  setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
  setsockopt(sock_fd, SOL_SOCKET, SO_REUSEPORT, &one, sizeof(one));
  if (auto e = bind(sock_fd, (struct sockaddr *)&addr, sizeof(addr)); e != 0) {
    perror("bind error");
    exit(-1);
  }

  bzero(&addr, sizeof(addr));
  if (!constructor_echo_addr(addr)) {
    exit(-1);
  }

  if (auto e = connect(sock_fd, (struct sockaddr *)&addr, sizeof(addr));
      e != 0) {
    perror("connect error");
    return -1;
  }

  int stand_id_flag = 0;
  fd_set set;
  char read_buf[1024] = {0};
  int count = 3;
  // for (;;) {
  //   if (stand_id_flag == 0) {
  //     FD_SET(fileno(stdin), &set);
  //   }
  //   FD_SET(sock_fd, &set);

  //   select(10, &set, nullptr, nullptr, 0);

  //   if (FD_ISSET(fileno(stdin), &set)) {
  //     bzero(read_buf, sizeof(read_buf));
  //     if (fgets(read_buf, sizeof(read_buf), stdin) == nullptr) {
  //       stand_id_flag = 1;
  //       shutdown(sock_fd, SHUT_WR);
  //       FD_CLR(fileno(stdin), &set);
  //       continue;
  //     }

  //     if (read_buf[0] == 'q') {
  //       close(sock_fd);
  //       perror("reconnect ...");
  //       goto reconnect;
  //     }

  //     if (write(sock_fd, read_buf, strlen(read_buf)) < 0) {
  //       perror("send error");
  //     }
  //   }

  //   if (FD_ISSET(sock_fd, &set)) {
  //     bzero(read_buf, sizeof(read_buf));
  //     if (auto e = read(sock_fd, read_buf, sizeof(read_buf)); e == 0) {
  //       close(sock_fd);
  //       perror("peer close");  // should be annotated
  //       exit(0);
  //     } else if (e < 0) {
  //       perror("read error");
  //       exit(-1);
  //     }
  //     fputs(read_buf, stdout);
  //   }
  // }
  return 0;
}

int get_local_ip(std::string_view dev) {
  int fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (fd < 0) {
    perror("create socket fail");
    return -1;
  }

  std::array<struct ifreq, 10> reqs;
  struct ifconf ifc;

  ifc.ifc_buf = reinterpret_cast<caddr_t>(reqs.data());
  ifc.ifc_len = reqs.size() * sizeof(struct ifreq);

  if (ioctl(fd, SIOCGIFCONF, &ifc) < 0) {
    perror("ioctl SIOCGIFCONF fail");
    return -1;
  }

  auto intrface_num = ifc.ifc_len / sizeof(struct ifreq);

  for (auto i = 0; i < intrface_num; i++) {
    if (ioctl(fd, SIOCGIFADDR, (char *)&reqs[i]) < 0) {
      perror("ioctl SIOCGIFADDR fail");
    }

    if (strcmp(reqs[i].ifr_ifrn.ifrn_name, dev.data()) == 0) {
      return ((struct sockaddr_in *)(&reqs[i].ifr_addr))->sin_addr.s_addr;
    }
  }

  return 0;
}

int main(int argc, char const *argv[]) {
  sigignore(SIGPIPE);

  std::vector<std::thread> client;

  auto devfd = open("./echo.doc.txt", O_RDONLY);
  if (devfd < 0) {
    perror("open dev list fail");
    return -1;
  }
  std::string dev_name;
  uint8_t buf[1];

  while (read(devfd, buf, 1)) {
    if (*buf != '\n') {
      dev_name.push_back(*buf);
    } else {
      auto ip = get_local_ip(dev_name);
      if (ip == 0) {
        goto clear;
      }
      client.emplace_back([=]() -> void {
        for (int j = 0; j < 28000; j++) {
          echo_client(ip);
          // sleep(1);
        }
      });
      client.back().detach();
    clear:
      dev_name.clear();
    }
  }

  while (1)
    ;
}