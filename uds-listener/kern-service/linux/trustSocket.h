#pragma once

#include <sys/socket.h>
#include <sys/types.h>

ssize_t trustRecv(int fd, void *buf, size_t size, int flags);