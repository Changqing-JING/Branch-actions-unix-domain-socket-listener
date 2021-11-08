#include <dlfcn.h>
#include <stdlib.h>
#include <sys/socket.h>
// #include <sys/types.h>
// #include <unistd.h>

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
  return origin_func(__domain, __type, __protocol);
}

extern int bind(int fd, const struct sockaddr *addr, socklen_t len) {
  typedef int (*libcall)(int fd, const struct sockaddr *addr, socklen_t len);
  GET_OLD_HANDLE(bind);
  return origin_func(fd, addr, len);
}

extern int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
  typedef int (*libcall)(int sockfd, const struct sockaddr *addr,
                         socklen_t addrlen);
  GET_OLD_HANDLE(connect);
  return origin_func(sockfd, addr, addrlen);
}

extern int listen(int sockfd, int backlog) {
  typedef int (*libcall)(int sockfd, int backlog);
  GET_OLD_HANDLE(listen);
  return origin_func(sockfd, backlog);
}

extern int accept(int fd, struct sockaddr *addr, socklen_t *len) {
  typedef int (*libcall)(int, struct sockaddr *, socklen_t *);
  GET_OLD_HANDLE(accept);
  return origin_func(fd, addr, len);
}

extern int accept4(int fd, struct sockaddr *addr, socklen_t *len, int flags) {
  typedef int (*libcall)(int, struct sockaddr *, socklen_t *, int);
  GET_OLD_HANDLE(accept4);
  return origin_func(fd, addr, len, flags);
}

extern ssize_t send(int fd, const void *buf, size_t size, int flags) {
  typedef ssize_t (*libcall)(int, const void *, size_t, int);
  GET_OLD_HANDLE(send);
  return origin_func(fd, buf, size, flags);
}

extern ssize_t recv(int fd, void *buf, size_t size, int flags) {
  typedef ssize_t (*libcall)(int, void *, size_t, int);
  GET_OLD_HANDLE(recv);
  return origin_func(fd, buf, size, flags);
}

extern ssize_t sendto(int fd, const void *buf, size_t size, int flags,
                      const struct sockaddr *addr, socklen_t len) {
  typedef ssize_t (*libcall)(int, const void *, size_t, int,
                             const struct sockaddr *, socklen_t);
  GET_OLD_HANDLE(sendto);
  return origin_func(fd, buf, size, flags, addr, len);
}

extern ssize_t recvfrom(int fd, void *buf, size_t size, int flags,
                        struct sockaddr *addr, socklen_t *len) {
  typedef ssize_t (*libcall)(int, void *, size_t, int, struct sockaddr *,
                             socklen_t *);
  GET_OLD_HANDLE(recvfrom);
  return origin_func(fd, buf, size, flags, addr, len);
}

extern ssize_t sendmsg(int fd, const struct msghdr *msg, int flags) {
  typedef ssize_t (*libcall)(int, const struct msghdr *, int);
  GET_OLD_HANDLE(sendmsg);
  return origin_func(fd, msg, flags);
}

extern ssize_t recvmsg(int fd, struct msghdr *msg, int flags) {
  typedef ssize_t (*libcall)(int, struct msghdr *, int);
  GET_OLD_HANDLE(recvmsg);
  return origin_func(fd, msg, flags);
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
  return origin_func(fd, buf, size);
}

extern ssize_t read(int fd, void *buf, size_t size) {
  typedef ssize_t (*libcall)(int, void *, size_t);
  GET_OLD_HANDLE(read);
  return origin_func(fd, buf, size);
}

extern int close(int fd) {
  typedef int (*libcall)(int);
  GET_OLD_HANDLE(close);
  return origin_func(fd);
}

extern int dup(int oldfd) {
  typedef int (*libcall)(int);
  GET_OLD_HANDLE(dup);
  return origin_func(oldfd);
}

extern int dup2(int oldfd, int newfd) {
  typedef int (*libcall)(int, int);
  GET_OLD_HANDLE(dup2);
  return origin_func(oldfd, newfd);
}

extern int dup3(int oldfd, int newfd, int flags) {
  typedef int (*libcall)(int, int, int);
  GET_OLD_HANDLE(dup3);
  return origin_func(oldfd, newfd, flags);
}

/*
 sys/uio.h - definitions for vector I/O operations
*/

/*
 sendfile.h - transfer data between file descriptors
 functions: sendfile()
*/
extern ssize_t sendfile(int out_fd, int in_fd, off_t *offset, size_t count) {
  typedef int (*libcall)(int out_fd, int in_fd, off_t *offset, size_t count);
  GET_OLD_HANDLE(sendfile);
  return origin_func(out_fd, in_fd, offset, count);
}