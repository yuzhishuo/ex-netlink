#pragma once

#ifndef LULUYUZHI_RTT_FD_H_
#define LULUYUZHI_RTT_FD_H_
#include <fcntl.h>
#include <inttypes.h>

#include <cstdint>

#include "IUnique.h"
namespace luluyuzhi::net {
class Fd : public tool::IUnique {
 public:
  Fd(uint32_t id, bool readenable, bool writeenable, bool errorenable)
      : read_{readenable}, write_{writeenable}, error_{errorenable}, id_{id} {}
  Fd(uint32_t id) : Fd(id, false, false, false) {}
  int getId() const override { return id_; }
  inline bool readenable() const noexcept { return read_; }
  inline bool writeenable() const noexcept { return write_; }
  inline bool errorenable() const noexcept { return error_; }
  inline bool responsable() const noexcept {
    return readenable() || writeenable() || errorenable();
  }

  inline void setReadenable() noexcept { read_ = true; update();}
  inline void setReaddisable() noexcept { read_ = false; update();}
  inline void setWriteenable() noexcept { write_ = true; update();}
  inline void setWritedisable() noexcept { write_ = false; update();}
  inline void setErrorenable() noexcept { error_ = true; update();}
  inline void setErrordisable() noexcept { error_ = false; update();}

 private:
 virtual void update() {};
 private:
  uint32_t read_ : 1;
  uint32_t write_ : 2;
  uint32_t error_ : 3;
  uint32_t padding : 4;
  uint32_t id_;
};

}  // namespace luluyuzhi::net

#endif  // LULUYUZHI_RTT_FD_H_