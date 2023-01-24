#pragma once

#include <cstdint>
#include <system_error>

namespace luluyuzhi::net {

class IHandle {
 public:
  virtual std::error_condition run(uint64_t timestamp) = 0;
};
}  // namespace luluyuzhi::net