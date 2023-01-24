#pragma once

namespace luluyuzhi::tool {

class IUnique {
 public:
  virtual int getId() const = 0;
};

inline int IUnique::getId() const { return 0; }
}  // namespace luluyuzhi::tool
