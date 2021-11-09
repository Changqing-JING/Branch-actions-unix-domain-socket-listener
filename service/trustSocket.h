#pragma once

#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>

ssize_t trustRecv(int fd, void *buf, size_t size, int flags) {
  ssize_t len = 0;
  do {
    len += recv(fd, ((char *)buf) + len, size - len, flags);
  } while (len != size && errno == EAGAIN);
  return len;
}