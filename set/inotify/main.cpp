#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/inotify.h>
#include <unistd.h>

/*
          IN_IGNORED
          Watch was removed explicitly (inotify_rm_watch(2)) or
          automatically (file was deleted, or filesystem was
          unmounted).  See also BUGS.
*/

#define EVENT_SIZE (sizeof(struct inotify_event))
#define BUF_LEN (1024 * (EVENT_SIZE + 16))

int main(int argc, char *argv[]) {
  int i, epfd, nfds, fd;
  int wd;
  int length;

  char buffer[BUF_LEN];
  struct epoll_event ev, events[20];
  epfd = epoll_create(256);
  fd = inotify_init();

  if (fd == -1) {
    perror("inotify_init fail");
    exit(EXIT_FAILURE);
  }

  wd = inotify_add_watch(fd, "/data/cc",
                         IN_MODIFY | IN_CREATE | IN_DELETE | IN_IGNORED |
                             IN_MOVED_TO | IN_OPEN | IN_CLOSE);
  if (wd == -1) {
    fprintf(stderr, "Cannot watch '%s': %s\n", argv[i], strerror(errno));
    exit(EXIT_FAILURE);
  }

  ev.data.fd = fd;
  ev.events = EPOLLIN | EPOLLET;

  epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);

  for (;;) {
    nfds = epoll_wait(epfd, events, 20, 100000);

    if (nfds == -1) {
      perror("epoll error");
      return -1;
    }

    for (i = 0; i < nfds; ++i) {
      if (events[i].data.fd == fd) {
        if (events[i].events & EPOLLIN) {
          length = read(fd, buffer, BUF_LEN);

          if (length < 0) {
            perror("read");
          }

          while (i < length) {
            struct inotify_event *event = (struct inotify_event *)&buffer[i];
            if (event->len) {
              printf("%s", event->name);
            }
            if (event->mask & IN_CREATE) {
              if (event->mask & IN_ISDIR) {
                printf("The directory %s was created.\n", event->name);
              } else {
                printf("The file %s was created.\n", event->name);
              }
            } else if (event->mask & IN_DELETE) {
              if (event->mask & IN_ISDIR) {
                printf("The directory %s was deleted.\n", event->name);
              } else {
                printf("The file %s was deleted.\n", event->name);
              }
            } else if (event->mask & IN_MODIFY) {
              if (event->mask & IN_ISDIR) {
                printf("The directory %s was modified.\n", event->name);
              } else {
                printf("The file %s was modified.\n", event->name);
              }
            } else if (event->mask & IN_IGNORED) {
              printf("The file %s was IN_IGNORED.\n", event->name);
            } else if (event->mask & IN_MOVED_TO) {
              printf("The file %s was IN_MOVED_TO.\n", event->name);
            } else if (event->mask & IN_OPEN) {
              printf("The file %s was IN_MOVED_TO.\n", event->name);
            } else if (event->mask & IN_CLOSE) {
              printf("The file %s was IN_MOVED_TO.\n", event->name);
            }
            i += EVENT_SIZE + event->len;
          }
        }
      }
    }
  }

  return 0;
}