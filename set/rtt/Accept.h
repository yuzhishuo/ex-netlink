#pragma once

#include <sys/types.h>

#include <system_error>
#ifndef LULUYUZHI_RTT_ACCEPT_H_
#define LULUYUZHI_RTT_ACCEPT_H_

#include <sys/sdt.h>

#include <functional>
#include <memory>

#include "Connector.h"
#include "Socket.h"
#include "Sockops.h"

namespace luluyuzhi::net {

class Accept : private Socket {
 public:
  explicit Accept(std::weak_ptr<IReactor> reactor, socket_model model)
      : Socket{reactor, model}, reactor_(reactor) {
    setReadFunction([this]() { create(); });
  }

  std::error_condition build(std::string_view ip, u_int32_t port) {
    DTRACE_PROBE2(accept, build, ip.data(), port);
    struct sockaddr_in peer_addr;
    peer_addr.sin_family = AF_INET;
    peer_addr.sin_port = htons(port);
    if (inet_net_pton(AF_INET, ip.data(), (void *)&peer_addr.sin_addr,
                      sizeof(peer_addr.sin_addr)) < 0) {
      perror("ip is seted fail");
      return {errno, std::system_category()};
    }

    if (bind(getId(), (struct sockaddr *)&peer_addr, sizeof(peer_addr)) < 0) {
      perror("bind fail");
      return {errno, std::system_category()};
    }

    net::listen(*this, 10);
    registor();
    setReaddisable();
    return {};
  }

  // FIXME: remove return value
  std::shared_ptr<Connector> create() {
    struct sockaddr_storage store;
    socklen_t len = sizeof(struct sockaddr_storage);
    int connect_fd;

    if (auto cond =
            net::accept(getId(), (struct sockaddr *)&store, &len, connect_fd);
        cond) {
      net::hold_error_condition(cond);
      return nullptr;
    }
    auto connector = std::make_shared<Connector>(connect_fd, reactor_, store,
                                                 socket_model::NORMAL);
    if (connector && on_new_connect_) {
      on_new_connect_(connector);
    }
    return connector;
  }

  void setOnNewConnect(
      std::function<void(std::shared_ptr<Connector> connector)> func) {
    on_new_connect_ = std::move(func);
  }

 private:
  std::weak_ptr<IReactor> reactor_;
  std::function<void(std::shared_ptr<Connector> connector)> on_new_connect_;
};
}  // namespace luluyuzhi::net

#endif  // LULUYUZHI_RTT_ACCEPT_H_