#pragma 1

#include <cstdint>
#include <functional>
#include <sys/epoll.h>

const int MAX_EPOLL_EVENT = 10;

class EpollServer {
private:
  int m_epoll_fd;
  int m_listen_fd;
  epoll_event m_events[MAX_EPOLL_EVENT];
  std::function<int(int)> m_on_recv_data;

public:
  EpollServer();
  ~EpollServer();
  void init_server(uint16_t port, std::function<int(int)> on_recv_data);
  void server_loop();
  void accept_new_client();
  void remove_client(int fd);

private:
  static void set_nonblock(int fd);
};
