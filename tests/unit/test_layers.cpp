#include <cstring>

#include <catch2/catch_test_macros.hpp>

#include "dmcp/doom/layer.h"
#include "dmcp/doom/protocol.h"

extern "C" {

dmcp_layer_t* dmcp_orchestrator_layer_create(void);
void          dmcp_orchestrator_layer_destroy(dmcp_layer_t* layer);

dmcp_layer_t* dmcp_input_layer_create(void);
void          dmcp_input_layer_destroy(dmcp_layer_t* layer);
}

TEST_CASE("Layer Registry: Lifecycle", "[layers][registry][lifecycle]") {
  SECTION("Create and destroy registry") {
    dmcp_layer_registry_t* registry = dmcp_layer_registry_create();
    REQUIRE(registry != nullptr);

    dmcp_layer_registry_destroy(registry);
  }

  SECTION("Destroy null registry is safe") { dmcp_layer_registry_destroy(nullptr); }

  SECTION("New registry has zero layers") {
    dmcp_layer_registry_t* registry = dmcp_layer_registry_create();
    REQUIRE(registry != nullptr);

    const size_t count = dmcp_layer_registry_count(registry);
    REQUIRE(count == 0);

    dmcp_layer_registry_destroy(registry);
  }
}

TEST_CASE("Layer Registry: Registration", "[layers][registry][registration]") {
  dmcp_layer_registry_t* registry = dmcp_layer_registry_create();
  REQUIRE(registry != nullptr);

  SECTION("Register null layer fails") {
    bool result = dmcp_layer_registry_register(registry, nullptr);
    REQUIRE(result == false);
  }

  SECTION("Register orchestrator layer succeeds") {
    dmcp_layer_t* orch = dmcp_orchestrator_layer_create();
    REQUIRE(orch != nullptr);

    bool result = dmcp_layer_registry_register(registry, orch);
    REQUIRE(result == true);
  }

  SECTION("Register input layer succeeds") {
    dmcp_layer_t* input = dmcp_input_layer_create();
    REQUIRE(input != nullptr);

    bool result = dmcp_layer_registry_register(registry, input);
    REQUIRE(result == true);
  }

  SECTION("Register duplicate layer fails") {
    dmcp_layer_t* orch = dmcp_orchestrator_layer_create();
    REQUIRE(orch != nullptr);

    REQUIRE(dmcp_layer_registry_register(registry, orch) == true);
    REQUIRE(dmcp_layer_registry_register(registry, orch) == false);
  }

  dmcp_layer_registry_destroy(registry);
}

TEST_CASE("Layer Registry: Enable/Disable", "[layers][registry][enable]") {
  dmcp_layer_registry_t* registry = dmcp_layer_registry_create();
  REQUIRE(registry != nullptr);

  dmcp_layer_t* orch = dmcp_orchestrator_layer_create();
  REQUIRE(orch != nullptr);

  REQUIRE(dmcp_layer_registry_register(registry, orch) == true);

  SECTION("Enable valid layer succeeds") {
    bool result = dmcp_layer_registry_enable(registry, DMCP_LAYER_ORCHESTRATOR);
    REQUIRE(result == true);
    REQUIRE(dmcp_layer_registry_is_enabled(registry, DMCP_LAYER_ORCHESTRATOR) == true);
  }

  SECTION("Disable valid layer succeeds") {
    REQUIRE(dmcp_layer_registry_enable(registry, DMCP_LAYER_ORCHESTRATOR) == true);

    bool result = dmcp_layer_registry_disable(registry, DMCP_LAYER_ORCHESTRATOR);
    REQUIRE(result == true);
    REQUIRE(dmcp_layer_registry_is_enabled(registry, DMCP_LAYER_ORCHESTRATOR) == false);
  }

  SECTION("Enable null name fails") {
    bool result = dmcp_layer_registry_enable(registry, nullptr);
    REQUIRE(result == false);
  }

  SECTION("Disable null name fails") {
    bool result = dmcp_layer_registry_disable(registry, nullptr);
    REQUIRE(result == false);
  }

  SECTION("Enable unknown layer fails") {
    bool result = dmcp_layer_registry_enable(registry, "nonexistent");
    REQUIRE(result == false);
  }

  SECTION("Disable unknown layer fails") {
    bool result = dmcp_layer_registry_disable(registry, "nonexistent");
    REQUIRE(result == false);
  }

  dmcp_layer_registry_destroy(registry);
}

TEST_CASE("Layer: Orchestrator Layer", "[layers][orchestrator]") {
  SECTION("Create and destroy orchestrator layer") {
    dmcp_layer_t* layer = dmcp_orchestrator_layer_create();
    REQUIRE(layer != nullptr);
    REQUIRE(layer->vtable != nullptr);
    REQUIRE(layer->vtable->name != nullptr);

    dmcp_orchestrator_layer_destroy(layer);
  }

  SECTION("Destroy null layer is safe") { dmcp_orchestrator_layer_destroy(nullptr); }

  SECTION("Layer has correct name") {
    dmcp_layer_t* layer = dmcp_orchestrator_layer_create();
    REQUIRE(layer != nullptr);

    const char* name = layer->vtable->name();
    REQUIRE(std::strcmp(name, DMCP_LAYER_ORCHESTRATOR) == 0);

    dmcp_orchestrator_layer_destroy(layer);
  }

  SECTION("Layer has description") {
    dmcp_layer_t* layer = dmcp_orchestrator_layer_create();
    REQUIRE(layer != nullptr);

    const char* desc = layer->vtable->description();
    REQUIRE(desc != nullptr);
    REQUIRE(std::strlen(desc) > 0);

    dmcp_orchestrator_layer_destroy(layer);
  }

  SECTION("Layer reports correct tool count") {
    dmcp_layer_t* layer = dmcp_orchestrator_layer_create();
    REQUIRE(layer != nullptr);

    size_t count = layer->vtable->tool_count();
    REQUIRE(count == 26);

    dmcp_orchestrator_layer_destroy(layer);
  }

  SECTION("Layer returns tool names") {
    dmcp_layer_t* layer = dmcp_orchestrator_layer_create();
    REQUIRE(layer != nullptr);

    const char* const* tools = layer->vtable->tools();
    REQUIRE(tools != nullptr);

    REQUIRE(std::strcmp(tools[0], DMCP_TOOL_GET_PLAYER) == 0);
    REQUIRE(std::strcmp(tools[1], DMCP_TOOL_GET_ENEMIES) == 0);
    REQUIRE(std::strcmp(tools[2], DMCP_TOOL_GET_ENTITIES) == 0);
    REQUIRE(std::strcmp(tools[9], DMCP_TOOL_GET_STATE_BATCH) == 0);
    REQUIRE(std::strcmp(tools[10], DMCP_TOOL_GET_SCREENSHOT) == 0);
    REQUIRE(std::strcmp(tools[11], DMCP_TOOL_EXECUTE_COMMAND) == 0);
    REQUIRE(std::strcmp(tools[12], DMCP_TOOL_GET_COMMAND_RESULT) == 0);
    REQUIRE(std::strcmp(tools[13], DMCP_TOOL_GET_AVAILABLE_CONTENT) == 0);
    REQUIRE(std::strcmp(tools[14], DMCP_TOOL_EXECUTE_BATCH) == 0);
    REQUIRE(std::strcmp(tools[15], DMCP_TOOL_GET_COMMAND_EXAMPLES) == 0);
    REQUIRE(std::strcmp(tools[16], DMCP_TOOL_SPAWN_ENTITY) == 0);
    REQUIRE(std::strcmp(tools[21], DMCP_TOOL_SET_PLAYER_POSITION) == 0);
    REQUIRE(std::strcmp(tools[25], DMCP_TOOL_KILL_ENTITY) == 0);

    dmcp_orchestrator_layer_destroy(layer);
  }
}

TEST_CASE("Layer: Input Layer", "[layers][input]") {
  SECTION("Create and destroy input layer") {
    dmcp_layer_t* layer = dmcp_input_layer_create();
    REQUIRE(layer != nullptr);
    REQUIRE(layer->vtable != nullptr);

    dmcp_input_layer_destroy(layer);
  }

  SECTION("Destroy null layer is safe") { dmcp_input_layer_destroy(nullptr); }

  SECTION("Layer has correct name") {
    dmcp_layer_t* layer = dmcp_input_layer_create();
    REQUIRE(layer != nullptr);

    const char* name = layer->vtable->name();
    REQUIRE(std::strcmp(name, DMCP_LAYER_INPUT) == 0);

    dmcp_input_layer_destroy(layer);
  }

  SECTION("Layer has description") {
    dmcp_layer_t* layer = dmcp_input_layer_create();
    REQUIRE(layer != nullptr);

    const char* desc = layer->vtable->description();
    REQUIRE(desc != nullptr);
    REQUIRE(std::strlen(desc) > 0);

    dmcp_input_layer_destroy(layer);
  }

  SECTION("Layer reports correct tool count") {
    dmcp_layer_t* layer = dmcp_input_layer_create();
    REQUIRE(layer != nullptr);

    size_t count = layer->vtable->tool_count();
    REQUIRE(count == 1);

    dmcp_input_layer_destroy(layer);
  }

  SECTION("Layer returns player_input tool") {
    dmcp_layer_t* layer = dmcp_input_layer_create();
    REQUIRE(layer != nullptr);

    const char* const* tools = layer->vtable->tools();
    REQUIRE(tools != nullptr);

    REQUIRE(std::strcmp(tools[0], DMCP_TOOL_PLAYER_INPUT) == 0);

    dmcp_input_layer_destroy(layer);
  }
}

TEST_CASE("Layer: Tool Namespacing", "[layers][namespacing]") {
  dmcp_layer_registry_t* registry = dmcp_layer_registry_create();
  REQUIRE(registry != nullptr);

  dmcp_layer_t* orch = dmcp_orchestrator_layer_create();
  REQUIRE(orch != nullptr);

  dmcp_layer_t* input = dmcp_input_layer_create();
  REQUIRE(input != nullptr);

  REQUIRE(dmcp_layer_registry_register(registry, orch) == true);
  REQUIRE(dmcp_layer_registry_register(registry, input) == true);

  SECTION("Each layer has unique tools") {
    const char* const* orch_tools = orch->vtable->tools();
    size_t             orch_count = orch->vtable->tool_count();

    const char* const* input_tools = input->vtable->tools();
    size_t             input_count = input->vtable->tool_count();

    for (size_t i = 0; i < orch_count; i++) {
      for (size_t j = 0; j < input_count; j++) {
        REQUIRE(std::strcmp(orch_tools[i], input_tools[j]) != 0);
      }
    }
  }

  SECTION("Tool names are not namespaced by default") {
    REQUIRE(std::strcmp(orch->vtable->name(), DMCP_LAYER_ORCHESTRATOR) == 0);
    REQUIRE(std::strcmp(input->vtable->name(), DMCP_LAYER_INPUT) == 0);

    const char* const* tools = orch->vtable->tools();
    REQUIRE(tools != nullptr);
    REQUIRE(std::strstr(tools[0], "orchestrator.") == nullptr);
  }

  dmcp_layer_registry_destroy(registry);
}

TEST_CASE("Layer: Multiple Layers", "[layers][multiple]") {
  dmcp_layer_registry_t* registry = dmcp_layer_registry_create();
  REQUIRE(registry != nullptr);

  dmcp_layer_t* orch = dmcp_orchestrator_layer_create();
  REQUIRE(orch != nullptr);

  dmcp_layer_t* input = dmcp_input_layer_create();
  REQUIRE(input != nullptr);

  REQUIRE(dmcp_layer_registry_register(registry, orch) == true);
  REQUIRE(dmcp_layer_registry_register(registry, input) == true);

  SECTION("Enable multiple layers") {
    REQUIRE(dmcp_layer_registry_enable(registry, DMCP_LAYER_ORCHESTRATOR) == true);
    REQUIRE(dmcp_layer_registry_enable(registry, DMCP_LAYER_INPUT) == true);

    REQUIRE(dmcp_layer_registry_disable(registry, DMCP_LAYER_ORCHESTRATOR) == true);
    REQUIRE(dmcp_layer_registry_disable(registry, DMCP_LAYER_INPUT) == true);
  }

  SECTION("Toggle layers independently") {
    REQUIRE(dmcp_layer_registry_enable(registry, DMCP_LAYER_ORCHESTRATOR) == true);
    REQUIRE(dmcp_layer_registry_enable(registry, DMCP_LAYER_INPUT) == true);

    REQUIRE(dmcp_layer_registry_disable(registry, DMCP_LAYER_ORCHESTRATOR) == true);
    REQUIRE(dmcp_layer_registry_disable(registry, DMCP_LAYER_INPUT) == true);
  }

  dmcp_layer_registry_destroy(registry);
}
