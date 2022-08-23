#include <arpa/inet.h>   // htons
#include <assert.h>      // assert
#include <errno.h>       // errno
#include <stdio.h>       // snprintf
#include <stdlib.h>      // atoi
#include <string.h>      // strerror,
#include <sys/socket.h>  // socket
#include <time.h>        // ctime
#include <unistd.h>      // write
#include <thread>

int main(int argc, char const *argv[]) {
  auto fd = socket(AF_INET, SOCK_STREAM, 0);

  if (fd < 0) {
    printf("socket error %s\n", strerror(errno));
    return -1;
  }

  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(13);

  if (auto bd = bind(fd, (sockaddr *)&addr, sizeof(addr)); bd < 0) {
    printf("bind error %s\n", strerror(errno));
  }

  char buff[256] = {0};

  if (auto l = listen(fd, 1); l < 0) {
    printf("listen error %s\n", strerror(errno));
  } else {
    for (;;) {
      struct sockaddr_in addr;
      bzero(&addr, sizeof(addr));
      socklen_t addrlen = sizeof(addr);
      if (auto cd = accept(fd, (sockaddr *)&addr, &addrlen); cd < 0) {
        printf("accept error %s\n", strerror(errno));
        continue;
      } else {
        if (auto pid = fork(); pid == 0) { // child process
          auto ticks = time(NULL);
          auto i = snprintf(buff, sizeof(buff), "%.24s\r\n", ctime(&ticks));
          assert(i != -1);
          auto write_size = write(cd, buff, sizeof(buff));
          assert(write_size == sizeof(buff));
          std::this_thread::sleep_for(std::chrono::seconds(10));
        }
      }
    }
  }

  /* code */
  return 0;
}
