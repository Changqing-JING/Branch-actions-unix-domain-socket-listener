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

#include "../common/interface.h"
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

void send_by_socket(int outfd, int fd, int isRead, const char *data,
                    int data_len) {
  struct serialize_socket_data *output_data =
      malloc(sizeof(serialize_socket_data) + data_len);
  output_data->fd = fd;
  memcpy(output_data->path, fd_status[fd].path, 108);
  output_data->isRead = isRead;
  output_data->data_len = data_len;
  memcpy(output_data->data, data, data_len);
  if (send(outfd, output_data, sizeof(serialize_socket_data) + data_len,
           MSG_NOSIGNAL) <= 0) {
    status = UNINIT | UNCONNECT;
    close(outfd);
  }
  free(output_data);
}

void send_name_by_socket(int fd) {
  pid_t pid = getpid();
  char proc_pid_path[256];
  char line_buf[256];
  static init_socket_data init_data;
  init_data.magic = MAGIC_CODE;
  sprintf(proc_pid_path, "/proc/%d/status", pid);
  FILE *fp = fopen(proc_pid_path, "r");
  if (NULL != fp && fgets(line_buf, 255, fp) != NULL) {
    int res = sscanf(line_buf, "Name:%255s", init_data.pid_name);
    if (res == EOF) {
      sprintf(init_data.pid_name, "%d", pid);
    }
    send(fd, &init_data, sizeof(init_data), MSG_NOSIGNAL);
  }
  fclose(fp);
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
    char *ip = getenv("UNIX_DOMAIN_SOCKET_FORWARD_IP");
    if (ip == NULL) {
      ip = "127.0.0.1";
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
    service_addr.sin_addr.s_addr = inet_addr(ip);
    service_addr.sin_port = htons(atoi(port));
  }

  if (status & UNCONNECT) {
    int old_errno = errno;
    if (-1 ==
        connect(fd, (struct sockaddr *)&service_addr, sizeof(service_addr))) {
      errno = old_errno;
      return 2;
    }
    send_name_by_socket(fd);
    status = 0;
    return fd;
  }

  return 2;
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
