#include "dmcp/binder.hpp"

namespace dmcp {

void Binder::Execute(Snapshot* snapshot) const {
  if (!snapshot) {
    return;
  }
  for (const auto& binding : bindings_) {
    binding(*snapshot);
  }
}

}  // namespace dmcp
