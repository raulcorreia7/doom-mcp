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
  bool is_enabled(const char* name) const;
  void tick_all(dmcp_context_t* ctx);
  void destroy_all_layers(void);

  size_t             layer_count() const;
  const layer_entry* get_layer(size_t index) const;
  const layer_entry* find_layer(const char* name) const;

 private:
  std::vector<layer_entry>                layers_;
  std::unordered_map<std::string, size_t> name_to_index_;
  mutable std::mutex                      mutex_;

  static void destroy_layer(dmcp_layer_t* layer);
};

}  // namespace dmcp
