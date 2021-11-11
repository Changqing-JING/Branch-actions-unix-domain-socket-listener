const int32_t MAGIC_CODE = -1234;

#pragma pack(1)
typedef struct serialize_socket_data {
  int32_t fd; // client -> server is rw fd, server -> UI is connection_fd
  char path[108];
  int32_t isRead;
  int32_t data_len;
  char data[0];
} serialize_socket_data;
typedef struct init_socket_data {
  int32_t magic;
  char pid_name[256];
} init_socket_data;
#pragma pack()