#pragma once
#ifndef EX_NETLINK_BILLBOARD_H
#define EX_NETLINK_BILLBOARD_H

#include <stdint.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <array>
#include <vector>

auto gettid()
{
  return (long int)syscall(224);
}


thread_local pid_t current_thread_id = gettid();

class Billboard {
 public:
  Billboard() {
    printf("Billboard\n");
    process_communication_init();
    current_thread_id_ = current_thread_id;
  }

  void start() {
    if (current_thread_id_ == current_thread_id) {
      printf("move Billboard in parent process %d\n", current_thread_id_);
      close(process_communication[0]);
      fd_ = process_communication[1];
      return;
    }
    close(process_communication[1]);
    fd_ = process_communication[0];
    current_thread_id_ = current_thread_id;
    printf("move Billboard in child process %d\n", current_thread_id_);
  }

  void send(std::vector<uint32_t>&);
  void recv(std::vector<uint32_t>&);

 private:
  int process_communication_init() {
    socketpair(AF_LOCAL, SOCK_STREAM, 0, process_communication.data());
    return 0;
  }

 private:
  std::array<int, 2> process_communication;
  pid_t current_thread_id_;
  int fd_ = 0;
};

#endif  // EX_NETLINK_BILLBOARD_H