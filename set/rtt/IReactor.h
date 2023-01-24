#pragma once

#include <cstdint>
#include <system_error>

#include "IHandle.h"
#include "IUnique.h"

namespace luluyuzhi::net {

class IReactor : public IHandle {
 public:
  virtual void add(const tool::IUnique& unique) = 0;
  virtual void remove(const tool::IUnique& unique) = 0;
  virtual void update(const tool::IUnique& unique) = 0;
};
}  // namespace luluyuzhi::net
