#pragma once
#ifndef LULUYUZHI_RTT_CONNECTOR_H_
#define LULUYUZHI_RTT_CONNECTOR_H_

#include <arpa/inet.h>

#include <any>
#include <type_traits>

#include "IReactor.h"
#include "Socket.h"

namespace luluyuzhi::net {
class Connector : public net::Socket {
 public:
  Connector(int fd, std::weak_ptr<IReactor> reactor,
            struct sockaddr_storage addr, socket_model model)
      : Socket(fd, reactor, model), addr_{addr} {}

  Connector(int fd, std::weak_ptr<IReactor> reactor, socket_model model)
      : Connector(fd, reactor, sockaddr_storage{}, model) {}

  template <typename T>
  void setData(T data) {
    data_ = data;
  }

  template <typename T>
  T getData() const {
    return data_;
  }

  template <typename T>
  T *getData() {
    return std::any_cast<T *>(data_);  // FIXME
  }

  int getId() const override { return Socket::getId(); }

  auto getRawHandler() const { return Socket::getId(); };

 private:
  struct sockaddr_storage addr_;
  std::any data_;
};
}  // namespace luluyuzhi::net

#endif  // LULUYUZHI_RTT_CONNECTOR_H_