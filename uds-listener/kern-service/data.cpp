#include "data.h"

#include <cstring>

Data::Data(serialize_socket_data &a, void *_data)
    : isRead(a.isRead), len(a.data_len) {
  std::memcpy(process_name, a.process_name, sizeof(process_name));
  std::memcpy(path, a.path, sizeof(path));
  if (len > 0) {
    p_data = std::malloc(a.data_len);
    std::memcpy(p_data, _data, len);
  } else {
    p_data = nullptr;
  }
}

void Data::copy(Data const &a) {
  std::memcpy(process_name, a.process_name, sizeof(process_name));
  isRead = a.isRead;
  len = a.len;
  std::memcpy(path, a.path, sizeof(path));
  if (p_data != nullptr) {
    free(p_data);
  }
  if (len > 0) {
    p_data = std::malloc(len);
    std::memcpy(p_data, a.p_data, len);
  } else {
    p_data = nullptr;
  }
}
void Data::move(Data &&a) {
  std::memcpy(process_name, a.process_name, sizeof(process_name));
  isRead = a.isRead;
  len = a.len;
  std::memcpy(path, a.path, sizeof(path));
  p_data = a.p_data;
  a.p_data = nullptr;
}