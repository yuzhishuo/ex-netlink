
#include <asm/types.h>
#include <configure.h>  // for CMAKE_PROJECT_VERSION_MAJOR, CMAKE_PROJECT_VERSION_MINOR
#include <linux/netlink.h>
#include <linux/rtnetlink.h>  // RTMGRP_LINK, RTMGRP_IPV4_IFADDR
#include <string.h>           // for memset
#include <sys/socket.h>       // for socket, AF_NETLINK,

#include <iostream>

int main(int argc, char** argv) {
  auto netlink_socket = socket(AF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE);

  struct sockaddr_nl sa;
  memset(&sa, 0, sizeof(sa));

  sa.nl_family = AF_NETLINK;
  sa.nl_groups = RTMGRP_LINK | RTMGRP_IPV4_IFADDR;

  bind(netlink_socket, (struct sockaddr*)&sa, sizeof(sa));

  // listen
  char buffer[1048] = {0};
  struct nlmsghdr* nlh;

  while (true) {
    fd_set readfds;

    FD_ZERO(&readfds);
    FD_SET(netlink_socket, &readfds);

    struct timeval timeout;
    memset(&timeout, 0, sizeof(timeout));

    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    if (auto ret = select(netlink_socket + 1, &readfds, NULL, NULL, &timeout);
        ret <= 0) {
      continue;
    }

    if (FD_ISSET(netlink_socket, &readfds)) {
      if (auto read_size = recv(netlink_socket, buffer, sizeof(buffer), 0);
          read_size < 0) {
        std::cout << "fail to recv." << std::endl;
      } else if (read_size == 0) {
        std::cout << "read 0 bytes" << std::endl;
      } else {
        nlh = (struct nlmsghdr*)buffer;
        for (; NLMSG_OK(nlh, read_size); nlh = NLMSG_NEXT(nlh, read_size)) {
          switch (nlh->nlmsg_type) {
            case NLMSG_DONE:
            case NLMSG_ERROR:
              break;
            case RTM_NEWLINK:
            case RTM_DELLINK:
              std::cout << "RTM_NEWLINK or RTM_DELLINK" << std::endl;
              break;
            case RTM_NEWADDR:
            case RTM_DELADDR:
              std::cout << "RTM_NEWLINK or RTM_DELLINK" << std::endl;
              break;
            default:
              break;
          }
        }
      }
    }
  }
  return 0;
}