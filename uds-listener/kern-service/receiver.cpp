#include <arpa/inet.h>
#include <cstring>

#include "receiver.hpp"

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
  std::cout << "popMessage\n";
  std::unique_lock<std::mutex> lo(m_mutex);
  m_cv.wait(lo, [this]() { return !this->m_recv_messages.empty(); });
  Data front = std::move(m_recv_messages.front());
  m_recv_messages.pop();
  return front;
}

void Receiver::serverloop() {
  std::cout << "serverloop\n";
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
  std::cout << "onReceiveData\n";
  serialize_socket_data dataHeader;
  ssize_t len = trustRecv(fd, reinterpret_cast<char *>(&dataHeader),
                          sizeof(dataHeader), MSG_WAITALL);
  if (len < sizeof(dataHeader)) {
    return CONNECT_ERROR;
  }
  dataHeader.isRead = ntohs(dataHeader.isRead);
  dataHeader.data_len = ntohl(dataHeader.data_len);
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