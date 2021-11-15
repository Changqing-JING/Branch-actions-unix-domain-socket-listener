
#pragma once

#include <cstdlib>
#include <utility>

#include "../../common/interface.h"

class Data {
public:
  char process_name[sizeof(((serialize_socket_data *)0)->process_name)];
  char path[sizeof(((serialize_socket_data *)0)->path)];
  decltype(((serialize_socket_data *)0)->isRead) isRead;
  decltype(((serialize_socket_data *)0)->data_len) len;
  void *p_data;
  Data() : p_data(nullptr){};
  Data(serialize_socket_data &_p, void *_data);
  Data(Data const &a) { copy(a); }
  Data &operator=(Data const &a) {
    copy(a);
    return *this;
  }
  Data(Data &&a) { move(std::move(a)); }
  Data &operator=(Data &&a) {
    move(std::move(a));
    return *this;
  }

  ~Data() {
    if (p_data != nullptr) {
      (std::free(p_data));
    }
  }

private:
  void copy(Data const &a);
  void move(Data &&a);
};
