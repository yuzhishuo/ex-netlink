#include <arpa/inet.h>
#include <errno.h>
#include <error.h>
#include <netinet/tcp.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

std::condition_variable condition;
std::mutex mutex;
std::atomic<bool> atom{false};

std::condition_variable condition1;
std::mutex mutex1;
std::atomic<bool> atom1{false};

void client() {
  auto fd = socket(AF_INET, SOCK_STREAM, 0);

  if (fd < 0) {
    printf("socket error %s \n", strerror(errno));
  }

  int flag = 1;
  int result = setsockopt(fd,            /* socket affected */
                          IPPROTO_TCP,   /* set option at TCP level */
                          TCP_NODELAY,   /* name of option */
                          (char *)&flag, /* the cast is historical cruft */
                          sizeof(int));  /* length of option value */

  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = inet_addr("127.0.0.1");
  addr.sin_port = htons(1234);

  std::unique_lock lk(mutex);
  condition.wait(lk, []() { return atom == true; });
  if (auto e = connect(fd, (struct sockaddr *)&addr, sizeof(addr)); e < 0) {
    printf("socket error %s\n", strerror(errno));
  } else {
    char buf[1024] = {0};

    if (auto ws1 = write(fd, buf, sizeof(buf)); ws1 < 0) {
      printf("write error %s \n", strerror(errno));
      close(fd);
      return;
    }
    std::unique_lock lk1(mutex1);
    condition1.wait(lk1, []() { return atom1 == true; });

    if (auto ws = write(fd, buf, sizeof(buf)); ws < 0) {
      printf("write error %s \n", strerror(errno));
      close(fd);
      return;
    }
    return;
  }
}

void server() {
  auto fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    printf("socket error %s \n", strerror(errno));
  }

  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = 0;
  addr.sin_port = htons(1234);
  int flag = 1;
  int result = setsockopt(fd,            /* socket affected */
                          IPPROTO_TCP,   /* set option at TCP level */
                          TCP_NODELAY,   /* name of option */
                          (char *)&flag, /* the cast is historical cruft */
                          sizeof(int));  /* length of option value */

  if (auto e = bind(fd, (struct sockaddr *)&addr, sizeof(addr)); e < 0) {
    printf("bind error %s \n", strerror(errno));
  }

  if (auto e = listen(fd, 100); e < 0) {
    printf("listen error %s \n ", strerror(errno));
  }

  atom = true;
  condition.notify_one();

  while (true) {
    struct sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    socklen_t si = sizeof(struct sockaddr_in);
    auto cd = accept(fd, (sockaddr *)&addr, &si);

    if (cd < 0) {
      printf("accept error %s", strerror(errno));
    }

    close(cd);
    atom1 = true;
    condition1.notify_one();
    printf("connect close\n");
  }
}

int main(int argc, char const *argv[]) {
  sigset_t sig;
  sigemptyset(&sig);
  sigaddset(&sig, SIGPIPE);
  sigprocmask(SIG_SETMASK, &sig, nullptr);

  std::thread t(client);

  std::thread t1(server);

  t.detach();
  t1.detach();
  while (1)
    ;
  return 0;
}
