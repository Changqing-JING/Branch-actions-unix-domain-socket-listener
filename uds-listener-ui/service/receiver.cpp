#include <cstring>

#include "receiver.h"

Data::Data(socket_data &_p, void *_data)
    : fd(_p.fd), isRead(_p.isRead), len(_p.data_len) {
  std::memcpy(path, _p.path, 108);
  p_data = std::malloc(_p.data_len);
  std::memcpy(p_data, _data, _p.data_len);
}

void Data::copy(Data const &a) {
  fd = a.fd;
  isRead = a.isRead;
  len = a.len;
  std::memcpy(path, a.path, 108);
  p_data = std::malloc(a.len);
  std::memcpy(p_data, a.p_data, a.len);
}
void Data::move(Data &&a) {
  fd = a.fd;
  isRead = a.isRead;
  len = a.len;
  std::memcpy(path, a.path, 108);
  p_data = a.p_data;
  a.p_data = nullptr;
}

void Receiver::start(uint16_t port) {
  {
    std::lock_guard<std::mutex> lo(m_mutex);
    if (m_status == ReceiverStatus::INIT || m_status == ReceiverStatus::STOP) {
      m_eps = new EpollServer{};
      m_eps->init_server(port,
                         [this](int fd) { return this->onReceiveData(fd); });
    } else if (m_status == ReceiverStatus::STOPING) {
    } else {
      throw std::runtime_error("start receiver under error status");
    }
    m_status = ReceiverStatus::START;
  }

  Eventloop::getInstance().insert_job(0,
                                      [this, port]() { this->serverloop(); });
}

Data Receiver::popMessage() {
  std::unique_lock<std::mutex> lo(m_mutex);
  m_cv.wait(lo, [this]() { return !this->m_recv_messages.empty(); });
  Data front = std::move(m_recv_messages.front());
  m_recv_messages.pop();
  return front;
}

void Receiver::serverloop() {
  m_eps->server_loop();
  if (m_status != ReceiverStatus::STOPING) {
    Eventloop::getInstance().insert_job(1000, [this]() { this->serverloop(); });
  } else {
    std::lock_guard<std::mutex> lo{m_mutex};
    delete m_eps;
    m_status = ReceiverStatus::STOP;
  }
}

int Receiver::onReceiveData(int fd) {
  socket_data dataHeader;
  int len = trustRecv(fd, &dataHeader, sizeof(dataHeader), MSG_WAITALL);
  if (len == 0) {
    return CONNECT_ERROR;
  }
  void *buf = malloc(dataHeader.data_len);
  len = trustRecv(fd, buf, dataHeader.data_len, MSG_WAITALL);
  if (len == 0) {
    free(buf);
    return CONNECT_ERROR;
  }
  {
    std::lock_guard<std::mutex> lo(m_mutex);
    m_recv_messages.emplace(std::move(Data{dataHeader, buf}));
  }
  free(buf);
  m_cv.notify_all();
  return CONNECT_OK;
}