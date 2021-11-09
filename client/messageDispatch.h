
#pragma once

#define WRITE_STREAM 0
#define READ_STREAM 1

void register_fd(int fd, int protocol);
void unregister_fd(int fd);
int get_fd_protocol(int fd);
void bind_addr(int fd, const struct sockaddr *addr);
void output(int fd, int isRead, const char *data, int data_len);