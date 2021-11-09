#include <dlfcn.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "messageDispatch.h"

#define GET_OLD_HANDLE(NAME)                                                   \
  static libcall origin_func = NULL;                                           \
  if (origin_func == NULL) {                                                   \
    void *libcHandle = NULL;                                                   \
    libcHandle = dlopen("libc.so.6", RTLD_LAZY);                               \
    origin_func = (libcall)dlsym(libcHandle, #NAME);                           \
  }

/**
 sys/socket.h - Internet Protocol family

 functions: socket(), bind(), connect(), listen(), accept(),
 accept4(), send(), recv(), sendto(), recvfrom(), sendmsg(),
 recvmsg(), sendmmsg(), recvmmsg(),
 */

extern int socket(int __domain, int __type, int __protocol) {
  typedef int (*libcall)(int domain, int type, int protocol);
  GET_OLD_HANDLE(socket);
  int fd = origin_func(__domain, __type, __protocol);
  register_fd(fd, __domain);
  return fd;
}

extern int bind(int fd, const struct sockaddr *addr, socklen_t len) {
  typedef int (*libcall)(int fd, const struct sockaddr *addr, socklen_t len);
  GET_OLD_HANDLE(bind);
  bind_addr(fd, addr);
  return origin_func(fd, addr, len);
}

extern int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
  typedef int (*libcall)(int sockfd, const struct sockaddr *addr,
                         socklen_t addrlen);
  GET_OLD_HANDLE(connect);
  bind_addr(sockfd, addr);
  return origin_func(sockfd, addr, addrlen);
}

// extern int listen(int sockfd, int backlog) {
//   typedef int (*libcall)(int sockfd, int backlog);
//   GET_OLD_HANDLE(listen);
//   return origin_func(sockfd, backlog);
// }

extern int accept(int fd, struct sockaddr *addr, socklen_t *len) {
  typedef int (*libcall)(int, struct sockaddr *, socklen_t *);
  GET_OLD_HANDLE(accept);
  int newfd = origin_func(fd, addr, len);
  register_fd(newfd, get_fd_protocol(fd));
  bind_addr(newfd, addr);
  return newfd;
}

extern int accept4(int fd, struct sockaddr *addr, socklen_t *len, int flags) {
  typedef int (*libcall)(int, struct sockaddr *, socklen_t *, int);
  GET_OLD_HANDLE(accept4);
  int newfd = origin_func(fd, addr, len, flags);
  register_fd(newfd, get_fd_protocol(fd));
  bind_addr(newfd, addr);
  return newfd;
}

extern ssize_t send(int fd, const void *buf, size_t size, int flags) {
  typedef ssize_t (*libcall)(int, const void *, size_t, int);
  GET_OLD_HANDLE(send);
  serialize(fd, WRITE_STREAM, buf, size);
  return origin_func(fd, buf, size, flags);
}

extern ssize_t recv(int fd, void *buf, size_t size, int flags) {
  typedef ssize_t (*libcall)(int, void *, size_t, int);
  GET_OLD_HANDLE(recv);
  ssize_t res = origin_func(fd, buf, size, flags);
  serialize(fd, READ_STREAM, buf, res);
  return res;
}

extern ssize_t sendto(int fd, const void *buf, size_t size, int flags,
                      const struct sockaddr *addr, socklen_t len) {
  typedef ssize_t (*libcall)(int, const void *, size_t, int,
                             const struct sockaddr *, socklen_t);
  GET_OLD_HANDLE(sendto);
  bind_addr(fd, addr);
  serialize(fd, WRITE_STREAM, buf, size);
  return origin_func(fd, buf, size, flags, addr, len);
}

extern ssize_t recvfrom(int fd, void *buf, size_t size, int flags,
                        struct sockaddr *addr, socklen_t *len) {
  typedef ssize_t (*libcall)(int, void *, size_t, int, struct sockaddr *,
                             socklen_t *);
  GET_OLD_HANDLE(recvfrom);
  ssize_t res = origin_func(fd, buf, size, flags, addr, len);
  bind_addr(fd, addr);
  serialize(fd, READ_STREAM, buf, res);
  return res;
}

extern ssize_t sendmsg(int fd, const struct msghdr *msg, int flags) {
  typedef ssize_t (*libcall)(int, const struct msghdr *, int);
  GET_OLD_HANDLE(sendmsg);
  for (int i = 0, k = msg->msg_iovlen; i < k; i++) {
    serialize(fd, WRITE_STREAM, msg->msg_iov[i].iov_base,
              msg->msg_iov[i].iov_len);
  }
  return origin_func(fd, msg, flags);
}

extern ssize_t recvmsg(int fd, struct msghdr *msg, int flags) {
  typedef ssize_t (*libcall)(int, struct msghdr *, int);
  GET_OLD_HANDLE(recvmsg);
  ssize_t res = origin_func(fd, msg, flags);
  ssize_t remainlen = res;
  for (int i = 0, k = msg->msg_iovlen; i < k; i++) {
    ssize_t msglen = msg->msg_iov[i].iov_len;
    if (remainlen >= msglen) {
      remainlen -= msglen;
      serialize(fd, READ_STREAM, msg->msg_iov[i].iov_base, msglen);
    } else {
      serialize(fd, READ_STREAM, msg->msg_iov[i].iov_base, remainlen);
      break;
    }
  }
  return res;
}

#ifdef __USE_GNU
extern int sendmmsg(int sockfd, struct mmsghdr *msgvec, unsigned int vlen,
                    int flags) {
  typedef ssize_t (*libcall)(int, struct mmsghdr *, unsigned int, int);
  GET_OLD_HANDLE(sendmmsg);
  return origin_func(sockfd, msgvec, vlen, flags);
}

extern int recvmmsg(int sockfd, struct mmsghdr *msgvec, unsigned int vlen,
                    int flags, struct timespec *timeout) {
  typedef ssize_t (*libcall)(int, struct mmsghdr *, unsigned int, int,
                             struct timespec *);
  GET_OLD_HANDLE(recvmmsg);
  return origin_func(sockfd, msgvec, vlen, flags, timeout);
}
#endif

/**
 unistd.h - standard symbolic constants and types

 functions: write(), read(), close(), fork(), dup(), dup2(), dup3()
 */

extern ssize_t write(int fd, const void *buf, size_t size) {
  typedef ssize_t (*libcall)(int, const void *, size_t);
  GET_OLD_HANDLE(write);
  serialize(fd, WRITE_STREAM, buf, size);
  return origin_func(fd, buf, size);
}

extern ssize_t read(int fd, void *buf, size_t size) {
  typedef ssize_t (*libcall)(int, void *, size_t);
  GET_OLD_HANDLE(read);
  ssize_t res = origin_func(fd, buf, size);
  serialize(fd, READ_STREAM, buf, res);
  return res;
}

extern int close(int fd) {
  typedef int (*libcall)(int);
  GET_OLD_HANDLE(close);
  unregister_fd(fd);
  return origin_func(fd);
}

extern int dup(int oldfd) {
  typedef int (*libcall)(int);
  GET_OLD_HANDLE(dup);
  int newfd = origin_func(oldfd);
  register_fd(newfd, get_fd_protocol(oldfd));
  return newfd;
}

extern int dup2(int oldfd, int newfd) {
  typedef int (*libcall)(int, int);
  GET_OLD_HANDLE(dup2);
  register_fd(newfd, get_fd_protocol(oldfd));
  return origin_func(oldfd, newfd);
}

extern int dup3(int oldfd, int newfd, int flags) {
  typedef int (*libcall)(int, int, int);
  GET_OLD_HANDLE(dup3);
  register_fd(newfd, get_fd_protocol(oldfd));
  return origin_func(oldfd, newfd, flags);
}

/*
 sys/uio.h - definitions for vector I/O operations
*/

/*
 sendfile.h - transfer data between file descriptors
 functions: sendfile()
*/
// extern ssize_t sendfile(int out_fd, int in_fd, off_t *offset, size_t count) {
//   typedef int (*libcall)(int out_fd, int in_fd, off_t *offset, size_t count);
//   GET_OLD_HANDLE(sendfile);
//   return origin_func(out_fd, in_fd, offset, count);
// }