#pragma once

#include <functional>
#include <vector>

#include "dmcp/schema.hpp"

namespace dmcp {

class Binder {
 public:
  using Binding = std::function<void(Snapshot&)>;

  void Bind(Binding binding) { bindings_.push_back(std::move(binding)); }
  void Reset() { bindings_.clear(); }
  void Execute(Snapshot* snapshot) const;

 private:
  std::vector<Binding> bindings_;
};

}  // namespace dmcp
