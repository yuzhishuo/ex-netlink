#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

using namespace std;

condition_variable cv;
mutex mt;
atomic_bool ck = {false};

void server() {
  auto fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd == -1) {
    printf("create socket err: %s \n", strerror(errno));
  }

  struct sockaddr_in addr;
  bzero(&addr, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(1234);
  addr.sin_addr.s_addr = INADDR_ANY;

  if (auto e = bind(fd, (struct sockaddr *)&addr, sizeof(addr)); e != 0) {
    printf("create bind err: %s \n", strerror(errno));
  }

  if (auto e = listen(fd, 100); e != 0) {
    printf("create listen err: %s \n", strerror(errno));
  }

  ck = true;
  cv.notify_one();
  while (true) {
    auto connectfd = accept(fd, nullptr, nullptr);
    if (connectfd == -1) {
      printf("create accept connectfd err: %s \n", strerror(errno));
    }

    char buf[1024] = {0};
    if (auto e = recv(connectfd, buf, sizeof(buf), 0); e == -1) {
      printf("recv err: %s \n", strerror(errno));
    }

    printf("recv msg : %s\n", buf);
    // close(connectfd);
  }
}

void client() {
  auto fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd == -1) {
    printf("create socket err: %s \n", strerror(errno));
  }

  struct sockaddr_in addr;
  bzero(&addr, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(4567);
  addr.sin_addr.s_addr = INADDR_ANY;

  if (auto e = bind(fd, (struct sockaddr *)&addr, sizeof(addr)); e != 0) {
    printf("create bind err: %s\n", strerror(errno));
  }

  struct sockaddr_in addr1;
  bzero(&addr, sizeof(addr1));
  addr1.sin_family = AF_INET;
  addr1.sin_port = htons(1234);
  addr1.sin_addr.s_addr = inet_addr("127.0.0.1");
  {
    std::unique_lock<std::mutex> ul(mt);
    cv.wait(ul, [&]() { return ck == true; });
  }
  if (auto e = connect(fd, (struct sockaddr *)&addr1, sizeof(addr1)); e != -1) {
    char buf[] = {'1', '2', '3', '4'};
    if (auto se = send(fd, buf, sizeof(buf), MSG_OOB); se == -1) {
      printf("create send err: %s \n", strerror(errno));
    }
    close(fd);
  } else {
    printf("create connect err: %s \n", strerror(errno));
  }
  while (1) {
  }
}

int main(int argc, char const *argv[]) {
  thread t1{server};
  thread t2{client};

  t1.detach();
  t2.detach();

  while (1) {
  }

  return 0;
}
