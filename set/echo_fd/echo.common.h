#ifndef ECHO_COMMON_H
#define ECHO_COMMON_H

#define REDIRECT_STDIN
#include <arpa/inet.h>
#include <netdb.h>  // getservbyname
#include <string.h>
#include <stdio.h>

constexpr auto NAMEDPIPENAME = "/tmp/NAMEDPIPENAME";
constexpr auto ECHOFILEPATH = "/workspaces/ex-netlink/set/echo/echo.doc.txt";

static constexpr auto server_name = "echo.server";

bool constructor_echo_addr(struct sockaddr_in& addr) {
  auto server_info = getservbyname("echo", "tcp");
  if (!server_info) return false;
  addr.sin_family = AF_INET;
  addr.sin_port = server_info->s_port;

  struct hostent* host = gethostbyname(server_name);
  if (!host) return false;
  if (host->h_addrtype == AF_INET && host->h_length == 4) {
    for (int i = 0; host->h_addr_list[i]; i++) {
      // printf("IP addr %d: %s\n", i + 1,
      //        inet_ntoa(*(struct in_addr*)host->h_addr_list[i]));
    }
    if (host->h_addr_list[0]) {
      memcpy(&addr.sin_addr, (struct in_addr*)host->h_addr_list[0],
             sizeof(in_addr));
    } else {
      printf("unfind server:%s", server_name);
      return false;
    }
  }
  return true;
}

#endif