#define __USE_POSIX
#define _GNU_SOURCE

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
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

pthread_mutex_t mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
volatile int init_mutex = 0;

void lock() { pthread_mutex_lock(&mutex); }
void unlock() { pthread_mutex_unlock(&mutex); }

typedef struct FDStatus {
  int protocol;
  char path[sizeof(((struct sockaddr_un *)0)->sun_path)];
} FDStatus;

static FDStatus fd_status[MAX_FD];

void register_fd(int fd, int protocol) {
  lock();
  if (fd >= 0 && fd < MAX_FD) {
    fd_status[fd].protocol = protocol;
  }
  unlock();
}
void unregister_fd(int fd) {
  lock();
  if (fd >= 0 && fd < MAX_FD) {
    fd_status[fd].protocol = -1;
  }
  unlock();
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
  lock();
  if (addr == NULL) {
    fd_status[fd].path[0] = '\0';
    unlock();
    return;
  }
  struct sockaddr_un *addr_un = (struct sockaddr_un *)addr;
  memcpy(fd_status[fd].path, addr_un->sun_path, sizeof(addr_un->sun_path));
  unlock();
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
  fprintf(stderr, "%s\n", out);
  free(out);
}

void getprocessname(char *process_name /* > 200 */) {
  pid_t pid = getpid();
  char proc_pid_path[256];
  char line_buf[256];
  sprintf(proc_pid_path, "/proc/%d/status", pid);
  FILE *fp = fopen(proc_pid_path, "r");
  if (fp == NULL) {
    sprintf(process_name, "%d", pid);
  } else if (fgets(line_buf, 255, fp) == NULL) {
    sprintf(process_name, "%d", pid);
  } else if (sscanf(line_buf, "Name:%200s", process_name) == EOF) {
    sprintf(process_name, "%d", pid);
  }
  fclose(fp);
}

void send_by_socket(int outfd, int fd, int isRead, const char *data,
                    int data_len /* > 0 */) {
  struct serialize_socket_data *output_data =
      malloc(sizeof(serialize_socket_data) + data_len);
  if (output_data == NULL) {
    return;
  }
  static int first = 1;
  static char processname[sizeof(((serialize_socket_data *)0)->process_name)];
  if (first == 1) {
    getprocessname(processname);
    first = 0;
  }
  memcpy(output_data->process_name, processname, sizeof(processname));
  memcpy(output_data->path, fd_status[fd].path, sizeof(fd_status[fd].path));
  output_data->isRead = htons(isRead);
  output_data->data_len = htonl(data_len);
  memcpy(output_data->data, data, data_len);
  if (send(outfd, output_data, sizeof(serialize_socket_data) + data_len,
           MSG_NOSIGNAL) <= 0) {
    perror("send failed");
    status = UNINIT | UNCONNECT;
    close(outfd);
  }
  free(output_data);
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
    if (port == NULL || strlen(port) == 0) {
      status = DEFAULT;
      printf("stderr\n");
      return 2;
    }
    printf("%s\n", port);
    char *ip = getenv("UNIX_DOMAIN_SOCKET_FORWARD_IP");
    if (ip == NULL || strlen(ip) == 0) {
      ip = "127.0.0.1";
    }
    printf("%s\n", ip);

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
      perror("connect");
      errno = old_errno;
      return 2;
    }
    status = 0;
    return fd;
  }

  return 2;
}

void output(int fd, int isRead, const char *data, int data_len) {
  if (!check_fd_unix(fd) || data_len <= 0) {
    return;
  }
  lock();
  int outfd = dispatch();
  if (outfd == 2) {
    send_by_stderr(fd, isRead, data, data_len);
  } else {
    send_by_socket(outfd, fd, isRead, data, data_len);
  }
  unlock();
}
