#define __USE_POSIX

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#include "messageDispatch.h"

#define MAX_FD 1024

typedef struct FDStatus {
  int protocol;
  char path[108];
} FDStatus;

static FDStatus fd_status[MAX_FD];

void register_fd(int fd, int protocol) {
  if (fd >= 0 && fd < MAX_FD) {
    fd_status[fd].protocol = protocol;
  }
}
void unregister_fd(int fd) {
  if (fd >= 0 && fd < MAX_FD) {
    fd_status[fd].protocol = -1;
  }
}
int get_fd_protocol(int fd) {
  if (fd >= 0 && fd < MAX_FD) {
    return fd_status[fd].protocol;
  } else {
    return -1;
  }
}
int check_fd_unix(int fd) {
  return fd >= 0 && fd < MAX_FD && fd_status[fd].protocol == AF_UNIX;
}

void bind_addr(int fd, const struct sockaddr *addr) {
  if (!check_fd_unix(fd)) {
    return;
  }
  if (addr == NULL) {
    fd_status[fd].path[0] = '\0';
    return;
  }
  struct sockaddr_un *addr_un = (struct sockaddr_un *)addr;
  memcpy(fd_status[fd].path, addr_un->sun_path, 108);
}

#define UNINIT 1
#define UNCONNECT 2
#define DEFAULT 4
static int status = UNINIT | UNCONNECT;

void send_by_stderr(int fd, int isRead, const char *data, int data_len) {
  char *out =
      malloc(4 /* fd */ + 1 + sizeof(FDStatus) - sizeof(int) /* path */ + 1 +
             1 /* R/W */ + 1 + 3 * data_len + 1);
  sprintf(out, "%4d\t%s\t%s\t", fd, fd_status[fd].path, isRead ? "R" : "W");
  char *now = out + strlen(out);
  for (int i = 0; i < data_len; i++) {
    sprintf(now, "%02x ", (unsigned char)(data[i]));
    now += 3;
  }
  sprintf(now, "\n");
  write(2, out, now - out + 1);
}

#pragma pack(1)
struct serilize_socket_data {
  int fd;
  char path[108];
  int isRead;
  int data_len;
  char data[0];
};
#pragma pack()

void send_by_socket(int outfd, int fd, int isRead, const char *data,
                    int data_len) {
  struct serilize_socket_data *out =
      malloc(sizeof(struct serilize_socket_data) + data_len);
  out->fd = fd;
  memcpy(out->path, fd_status[fd].path, 108);
  out->isRead = isRead;
  out->data_len = data_len;
  memcpy(out->data, data, data_len);
  if (send(2, out, sizeof(struct serilize_socket_data) + data_len,
           MSG_NOSIGNAL) <= 0) {
    status = UNINIT | UNCONNECT;
    close(fd);
  }
}

int dispatch() {
  static int fd = -1;
  static struct sockaddr_in service_addr;

  // output routing done
  if (status == 0) {
    return fd;
  }
  if (status == DEFAULT) {
    return 2;
  }

  // init
  if (status & UNINIT) {
    char *port = getenv("UNIX_DOMAIN_SOCKET_FORWARD_PORT");
    if (port == NULL) {
      status = DEFAULT;
      return 2;
    }

    status -= UNINIT;
    int old_errno = errno;
    if (-1 == (fd = socket(AF_INET, SOCK_STREAM, 0))) {
      perror("socket failed");
      errno = old_errno;
      return 2;
    }
    memset(&service_addr, 0, sizeof(service_addr));
    service_addr.sin_family = AF_INET;
    service_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    service_addr.sin_port = htons(atoi(port));
    if (-1 ==
        connect(fd, (struct sockaddr *)&service_addr, sizeof(service_addr))) {
      errno = old_errno;
      return 2;
    }
    status = 0;
    return fd;
  }

  if (status & UNCONNECT) {
    int old_errno = errno;
    if (-1 ==
        connect(fd, (struct sockaddr *)&service_addr, sizeof(service_addr))) {
      errno = old_errno;
      return 2;
    }
    status = 0;
    return fd;
  }
}

void output(int fd, int isRead, const char *data, int data_len) {
  if (!check_fd_unix(fd)) {
    return;
  }
  int outfd = dispatch();
  if (outfd == 2) {
    send_by_stderr(fd, isRead, data, data_len);
  } else {
    send_by_socket(outfd, fd, isRead, data, data_len);
  }
}
