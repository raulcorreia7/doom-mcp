#pragma once

#include <memory>
#include <string>
#include <vector>

#include "dmcp/doom/layer.h"
#include "dmcp/doom/protocol.h"
#include "mcp/generic/server.h"

#ifdef __cplusplus
extern "C" {
#endif

namespace dmcp {

struct context;

class orchestrator_layer {
 public:
  explicit orchestrator_layer();
  ~orchestrator_layer();

  static const char*  layer_name();
  static const char*  layer_description();
  static size_t       tool_count();
  static const char** tools();

  bool register_methods(mcp_server_t* server, void* user_data);
  bool register_routes(mcp_server_t* server, void* user_data);
  void tick(dmcp_context_t* ctx);

 private:
  context* m_ctx = nullptr;
};

dmcp_layer_t* create_orchestrator_layer();
void          destroy_orchestrator_layer(dmcp_layer_t* layer);

}  // namespace dmcp

DMCP_API dmcp_layer_t* dmcp_orchestrator_layer_create(void);
DMCP_API void          dmcp_orchestrator_layer_destroy(dmcp_layer_t* layer);

#ifdef __cplusplus
}
#endif
