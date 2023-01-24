#pragma once

#include <cstdint>
#include <iostream>
#include <iterator>
#include <memory>
#include <system_error>

#include "Fd.h"
#include "IReactor.h"
#include "IUnique.h"
#include "Sockops.h"

namespace luluyuzhi::net {

enum class socket_model : uint8_t { LISTEN_SHARED, LISTEN_EXCLUSE, NORMAL };

class Socket : public Fd , public std::enable_shared_from_this<Socket> {
 public:
  explicit Socket(uint32_t id, std::weak_ptr<IReactor> reactor,
                  socket_model model)
      : Fd{id}, id_(id), model_(model), reactor_(reactor) {}

  explicit Socket(std::weak_ptr<IReactor> reactor, socket_model model)
      : Socket{static_cast<uint32_t>([]() {
                 int sock_fd_;
                 net::createSocket(sock_fd_);
                 return sock_fd_;
               }()),
               reactor, model} {}

  int getId() const override { return id_; }

  socket_model getModel() const { return model_; }
  
  void shutdonw() const {
    net::hold_error_condition(net::shutdown(id_, net::ShutType::WR));
  }
  void close() const { net::hold_error_condition(net::close(id_)); }

  bool registor() {
    auto r = reactor_.lock();
    if (!r) return false;
    r->add(*this);
    return true;
  }

  inline void setReadFunction(std::function<void()> fun) noexcept {
    read_fun_ = std::move(fun);
  }

  inline void setWriteFunction(std::function<void()> fun) noexcept {
    write_fun_ = std::move(fun);
  }

 private:
  void update() override {
    auto r = reactor_.lock();
    if (!r) return;
    r->update(*this);
  }

 private:
 private:
  int id_;
  socket_model model_;
  std::weak_ptr<IReactor> reactor_;
  std::function<void()> read_fun_;
  std::function<void()> write_fun_;
};
}  // namespace luluyuzhi::net
