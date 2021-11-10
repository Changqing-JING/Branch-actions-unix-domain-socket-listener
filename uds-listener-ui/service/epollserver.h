#pragma 1

#include <cstdint>
#include <functional>
#include <set>
#include <sys/epoll.h>

#define CONNECT_ERROR -1
#define CONNECT_OK 0

const int MAX_EPOLL_EVENT = 10;

class EpollServer {
private:
  int m_epoll_fd;
  int m_listen_fd;
  std::set<int> m_connect_fds;
  epoll_event m_events[MAX_EPOLL_EVENT];
  std::function<int(int)> m_on_recv_data;

public:
  EpollServer();
  ~EpollServer();
  void init_server(uint16_t port, std::function<int(int)> on_recv_data);
  void server_loop();
  void accept_new_client();
  void clear_fd(int fd);
  void remove_client(int fd);

private:
  static void set_nonblock(int fd);
};
