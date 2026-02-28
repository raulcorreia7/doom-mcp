#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "dmcp/doom/layer.h"
#include "dmcp/doom/types.h"

namespace dmcp {

struct layer_entry {
  dmcp_layer_t* layer       = nullptr;
  bool          enabled     = false;
  const char*   name        = nullptr;
  const char*   description = nullptr;
  size_t        tool_count  = 0;
};

class layer_registry {
 public:
  layer_registry();
  ~layer_registry();

  bool register_layer(dmcp_layer_t* layer);
  bool enable(const char* name);
  bool disable(const char* name);
  void tick_all(dmcp_context_t* ctx);

  size_t             layer_count() const;
  const layer_entry* get_layer(size_t index) const;
  const layer_entry* find_layer(const char* name) const;

 private:
  std::vector<layer_entry>                layers_;
  std::unordered_map<std::string, size_t> name_to_index_;
  mutable std::mutex                      mutex_;
};

}  // namespace dmcp

extern "C" {

DMCP_API dmcp_layer_registry_t* dmcp_layer_registry_create(void);
DMCP_API void                   dmcp_layer_registry_destroy(dmcp_layer_registry_t* registry);
DMCP_API bool dmcp_layer_registry_register(dmcp_layer_registry_t* registry, dmcp_layer_t* layer);
DMCP_API bool dmcp_layer_registry_enable(dmcp_layer_registry_t* registry, const char* name);
DMCP_API bool dmcp_layer_registry_disable(dmcp_layer_registry_t* registry, const char* name);
DMCP_API void dmcp_layer_registry_tick_all(dmcp_layer_registry_t* registry, dmcp_context_t* ctx);
}
