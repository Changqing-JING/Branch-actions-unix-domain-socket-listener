#pragma once

#include <stdint.h>

const int32_t MAGIC_CODE = -1234;

#pragma pack(1)
typedef struct serialize_socket_data {
  char process_name[256];
  char path[256];
  uint32_t data_len;
  uint16_t isRead;
  char data[0];
} serialize_socket_data;
#pragma pack()