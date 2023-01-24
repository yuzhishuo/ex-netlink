#pragma once
#include <pthread.h>
#ifndef EX_NETLINK_BILLBOARD_H
#define EX_NETLINK_BILLBOARD_H

#include <stdint.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <array>
#include <vector>

thread_local pthread_t current_thread_id = pthread_self();

class Billboard {
 public:
  Billboard() {
    printf("Billboard\n");
    process_communication_init();
    current_thread_id_ = current_thread_id;
  }

  void start() {
    if (current_thread_id_ == current_thread_id) {
      printf("move Billboard in parent process %lu\n", current_thread_id_);
      close(process_communication_[0]);
      fd_ = process_communication_[1];
      return;
    }
    close(process_communication_[1]);
    fd_ = process_communication_[0];
    current_thread_id_ = current_thread_id;
    printf("move Billboard in child process %lu\n", current_thread_id_);
  }

  void send(std::vector<uint32_t>&);
  void recv(std::vector<uint32_t>&);

 private:
  int process_communication_init() {
    socketpair(AF_LOCAL, SOCK_STREAM, 0, process_communication_.data());
    return 0;
  }

 private:
  std::array<int, 2> process_communication_;
  pthread_t current_thread_id_;
  int fd_ = 0;
};

#endif  // EX_NETLINK_BILLBOARD_H