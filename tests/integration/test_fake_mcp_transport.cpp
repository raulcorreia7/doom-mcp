#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include "dmcp/adapters/fake.h"
#include "dmcp/doom/protocol.h"
#include "mcp/generic/constants.h"
#include "mcp/json/json.hpp"
#include "support/mcp_http_client.hpp"
#include "support/network.hpp"

using namespace std::chrono_literals;

namespace {

struct fake_server_fixture {
  uint16_t     port = 0;
  dmcp_fake_t* fake = nullptr;

  fake_server_fixture() {
    port = dmcp::test::allocate_loopback_port();
    REQUIRE(port != 0);

    dmcp_fake_config_t config     = dmcp_fake_config_default();
    config.base.port              = port;
    config.base.start_transport   = true;
    config.base.screenshot.enable = false;
    config.base.tools.game        = true;
    config.base.tools.input       = true;

    fake = dmcp_fake_create(&config);
    REQUIRE(fake != nullptr);
    REQUIRE(dmcp::test::wait_for_health(port, 2s));
  }

  ~fake_server_fixture() { dmcp_fake_destroy(fake); }

  void tick(int count = 3) {
    for (int i = 0; i < count; ++i) {
      const mcp_status_t result = dmcp_fake_tick(fake);
      REQUIRE(result.code == MCP_STATUS_CODE_OK);
      std::this_thread::sleep_for(35ms);
    }
  }
};

bool body_has_text(const std::string& body, const char* needle) {
  return body.find(needle) != std::string::npos;
}

bool string_equals(std::string_view actual, const char* expected) {
  return actual == std::string_view(expected);
}

std::string call_jsonrpc(uint16_t port, const std::string& session_id, int id, const char* method,
                         const char* params) {
  const std::string body = dmcp::test::jsonrpc_request(id, method, params);
  const auto        res  = dmcp::test::HttpPost(port, MCP_ENDPOINT_MCP, body.c_str(),
                                                dmcp::test::SessionHeaders(session_id));
  REQUIRE(res.status == 200);
  return res.body;
}

mcp::json::Value require_jsonrpc_result(const std::string& body, mcp::json::Document& doc) {
  REQUIRE(doc.parse(body));
  auto root = doc.root();
  REQUIRE(root.is_object());
  REQUIRE(root.has_member("result"));
  REQUIRE_FALSE(root.has_member("error"));
  return root["result"];
}

mcp::json::Value require_jsonrpc_error(const std::string& body, mcp::json::Document& doc) {
  REQUIRE(doc.parse(body));
  auto root = doc.root();
  REQUIRE(root.is_object());
  REQUIRE(root.has_member("error"));
  REQUIRE_FALSE(root.has_member("result"));
  auto error = root["error"];
  REQUIRE(error.is_object());
  return error;
}

mcp::json::Value require_tool_json_payload(const std::string& body, mcp::json::Document& doc) {
  const std::string text = dmcp::test::tool_text_payload(body);
  REQUIRE(!text.empty());
  REQUIRE(doc.parse(text));
  auto root = doc.root();
  REQUIRE(root.is_object());
  return root;
}

mcp::json::Value find_tool_by_name(const mcp::json::Value& tools, const char* name) {
  REQUIRE(tools.is_array());
  for (size_t i = 0; i < tools.size(); ++i) {
    auto tool = tools[i];
    REQUIRE(tool.is_object());
    auto name_value = tool["name"];
    REQUIRE(name_value.is_string());
    if (string_equals(name_value.get_string(), name)) {
      return tool;
    }
  }
  return {};
}

int64_t extract_sequence_from_tool_payload(const std::string& body) {
  const std::string text = dmcp::test::tool_text_payload(body);
  REQUIRE(!text.empty());

  mcp::json::Document doc;
  REQUIRE(doc.parse(text));
  auto root = doc.root();
  REQUIRE(root.has_member("sequence"));
  return root["sequence"].get_int();
}

std::vector<int64_t> extract_sequences_from_batch_payload(const std::string& body) {
  const std::string text = dmcp::test::tool_text_payload(body);
  REQUIRE(!text.empty());

  mcp::json::Document doc;
  REQUIRE(doc.parse(text));
  auto root = doc.root();
  REQUIRE(root.has_member("sequences"));
  REQUIRE(root["sequences"].is_array());

  std::vector<int64_t> sequences;
  for (size_t i = 0; i < root["sequences"].size(); ++i) {
    sequences.push_back(root["sequences"][i].get_int());
  }
  return sequences;
}

}  // namespace

TEST_CASE("Fake adapter exposes SDK MCP transport", "[integration][sdk][fake][transport]") {
  fake_server_fixture server;
  server.tick();

  SECTION("health and direct game state route respond") {
    auto client = dmcp::test::make_client(server.port);

    const auto health = client.Get(MCP_ENDPOINT_HEALTH);
    REQUIRE(health);
    REQUIRE(health->status == 200);
    REQUIRE(body_has_text(health->body, R"("status":"ok")"));

    const auto state = client.Get("/game/state");
    REQUIRE(state);
    REQUIRE(state->status == 200);
    REQUIRE(body_has_text(state->body, R"("player")"));
    REQUIRE(body_has_text(state->body, R"("level")"));
    REQUIRE(body_has_text(state->body, R"("game")"));
  }

  SECTION("initialize, list tools, and read player state") {
    const std::string session_id = dmcp::test::initialize_session(server.port);
    REQUIRE(!session_id.empty());

    const std::string   tools = call_jsonrpc(server.port, session_id, 2, "tools/list", "{}");
    mcp::json::Document tools_doc;
    auto                tools_result = require_jsonrpc_result(tools, tools_doc);
    auto                tool_entries = tools_result["tools"];
    REQUIRE(tool_entries.is_array());

    const char* const expected_tools[] = {
        DMCP_TOOL_GET_PLAYER,
        DMCP_TOOL_GET_ENEMIES,
        DMCP_TOOL_GET_ENTITIES,
        DMCP_TOOL_GET_ITEMS,
        DMCP_TOOL_GET_MAP,
        DMCP_TOOL_GET_INVENTORY,
        DMCP_TOOL_GET_GAME_INFO,
        DMCP_TOOL_GET_STATE,
        DMCP_TOOL_GET_STATE_BATCH,
        DMCP_TOOL_GET_COMMAND_RESULT,
        DMCP_TOOL_GET_AVAILABLE_CONTENT,
        DMCP_TOOL_GET_AVAILABLE_ENEMIES,
        DMCP_TOOL_GET_AVAILABLE_ENTITIES,
        DMCP_TOOL_GET_AVAILABLE_ITEMS,
        DMCP_TOOL_GET_AVAILABLE_WEAPONS,
        DMCP_TOOL_GET_AVAILABLE_AMMO,
        DMCP_TOOL_GET_AVAILABLE_KEYS,
        DMCP_TOOL_GET_AVAILABLE_MAPS,
        DMCP_TOOL_GET_AVAILABLE_GIVEABLE,
        DMCP_TOOL_EXECUTE_BATCH,
        DMCP_TOOL_GET_COMMAND_EXAMPLES,
        DMCP_TOOL_GIVE_ITEM,
        DMCP_TOOL_SPAWN_ENTITY,
        DMCP_TOOL_CHANGE_LEVEL,
        DMCP_TOOL_SET_PLAYER_HEALTH,
        DMCP_TOOL_SET_PLAYER_POSITION,
        DMCP_TOOL_EXECUTE_CONSOLE,
        DMCP_TOOL_PAUSE_GAME,
        DMCP_TOOL_DAMAGE_ENTITY,
        DMCP_TOOL_KILL_ENTITY,
        DMCP_TOOL_PLAYER_INPUT,
    };
    for (const char* expected_tool : expected_tools) {
      auto tool = find_tool_by_name(tool_entries, expected_tool);
      REQUIRE(tool);
      REQUIRE(tool["description"].is_string());
      REQUIRE(tool["inputSchema"].is_object());
    }

    REQUIRE_FALSE(find_tool_by_name(tool_entries, "execute_command"));
    REQUIRE_FALSE(find_tool_by_name(tool_entries, "set_health"));
    REQUIRE_FALSE(find_tool_by_name(tool_entries, "set_position"));
    REQUIRE_FALSE(find_tool_by_name(tool_entries, "console_command"));

    const std::string player  = call_jsonrpc(server.port, session_id, 3, "tools/call",
                                             R"({"name":"get_player","arguments":{}})");
    const std::string payload = dmcp::test::tool_text_payload(player);
    REQUIRE(body_has_text(payload, R"("hp")"));
    REQUIRE(body_has_text(payload, R"("position")"));
  }

  SECTION("canonical granular state tools accept structured filters") {
    const std::string session_id = dmcp::test::initialize_session(server.port);
    REQUIRE(!session_id.empty());

    const std::string entities = call_jsonrpc(
        server.port, session_id, 10, "tools/call",
        R"({"name":"get_entities","arguments":{"kind":"enemy","status":"alive","limit":2}})");
    mcp::json::Document entities_doc;
    auto                entities_payload = require_tool_json_payload(entities, entities_doc);
    REQUIRE(string_equals(entities_payload["kind"].get_string(), "enemy"));
    REQUIRE(string_equals(entities_payload["status"].get_string(), "alive"));
    REQUIRE(entities_payload["entity_count"].get_int() == 3);
    REQUIRE(entities_payload["returned"].get_int() == 2);
    auto entity_items = entities_payload["entities"];
    REQUIRE(entity_items.is_array());
    REQUIRE(entity_items.size() == 2);
    REQUIRE(string_equals(entity_items[0]["type"].get_string(), "DoomImp"));
    REQUIRE(string_equals(entity_items[0]["kind"].get_string(), "enemy"));
    REQUIRE(string_equals(entity_items[0]["state"].get_string(), "alive"));
    REQUIRE(string_equals(entity_items[1]["type"].get_string(), "ShotgunGuy"));
    REQUIRE(string_equals(entity_items[1]["kind"].get_string(), "enemy"));

    const std::string items =
        call_jsonrpc(server.port, session_id, 11, "tools/call",
                     R"({"name":"get_items","arguments":{"kind":"armor","limit":1}})");
    mcp::json::Document items_doc;
    auto                items_payload = require_tool_json_payload(items, items_doc);
    REQUIRE(string_equals(items_payload["kind"].get_string(), "armor"));
    REQUIRE(items_payload["item_count"].get_int() == 1);
    REQUIRE(items_payload["returned"].get_int() == 1);
    auto item_entries = items_payload["items"];
    REQUIRE(item_entries.is_array());
    REQUIRE(item_entries.size() == 1);
    REQUIRE(string_equals(item_entries[0]["type"].get_string(), "ArmorBonus"));
    REQUIRE(string_equals(item_entries[0]["kind"].get_string(), "armor"));
  }

  SECTION("removed native JSON-RPC aliases are rejected") {
    const std::string session_id = dmcp::test::initialize_session(server.port);
    REQUIRE(!session_id.empty());

    const std::string player_alias = call_jsonrpc(server.port, session_id, 12, "get_player", "{}");
    mcp::json::Document player_error_doc;
    auto                player_error = require_jsonrpc_error(player_alias, player_error_doc);
    REQUIRE(player_error["code"].get_int() == -32601);
    REQUIRE(string_equals(player_error["message"].get_string(), "Method not found"));

    const std::string command_alias =
        call_jsonrpc(server.port, session_id, 13, "execute_command",
                     R"({"type":"pause_game","params":{"paused":true}})");
    mcp::json::Document command_error_doc;
    auto                command_error = require_jsonrpc_error(command_alias, command_error_doc);
    REQUIRE(command_error["code"].get_int() == -32601);
    REQUIRE(string_equals(command_error["message"].get_string(), "Method not found"));
  }

  SECTION("available content exposes canonical compact catalog") {
    const std::string session_id = dmcp::test::initialize_session(server.port);
    REQUIRE(!session_id.empty());

    const std::string content = call_jsonrpc(server.port, session_id, 4, "tools/call",
                                             R"({"name":"get_available_content","arguments":{}})");
    const std::string payload = dmcp::test::tool_text_payload(content);
    REQUIRE(body_has_text(payload, R"("catalog_version":1)"));
    REQUIRE(body_has_text(payload, R"("enemies")"));
    REQUIRE(body_has_text(payload, R"("weapons")"));
    REQUIRE(body_has_text(payload, R"("ammo")"));
    REQUIRE(body_has_text(payload, R"("keys")"));
    REQUIRE(body_has_text(payload, R"("giveable")"));
    REQUIRE_FALSE(body_has_text(payload, R"("monsters")"));
    REQUIRE_FALSE(body_has_text(payload, "Shotgun Shells"));
  }

  SECTION("granular available content tools expose available-only canonical names") {
    const std::string session_id = dmcp::test::initialize_session(server.port);
    REQUIRE(!session_id.empty());

    const std::string enemies =
        call_jsonrpc(server.port, session_id, 14, "tools/call",
                     R"({"name":"get_available_enemies","arguments":{"game_mode":"shareware"}})");
    const std::string enemies_payload = dmcp::test::tool_text_payload(enemies);
    REQUIRE(body_has_text(enemies_payload, R"("catalog_scope":"enemies")"));
    REQUIRE(body_has_text(enemies_payload, "DoomImp"));
    REQUIRE_FALSE(body_has_text(enemies_payload, "Cyberdemon"));

    const std::string maps =
        call_jsonrpc(server.port, session_id, 15, "tools/call",
                     R"({"name":"get_available_maps","arguments":{"game_mode":"shareware"}})");
    const std::string maps_payload = dmcp::test::tool_text_payload(maps);
    REQUIRE(body_has_text(maps_payload, R"("catalog_scope":"maps")"));
    REQUIRE(body_has_text(maps_payload, "MAP01"));
    REQUIRE(body_has_text(maps_payload, "MAP02"));
    REQUIRE_FALSE(body_has_text(maps_payload, "E1M1"));

    const std::string giveable =
        call_jsonrpc(server.port, session_id, 16, "tools/call",
                     R"({"name":"get_available_giveable","arguments":{"game_mode":"retail"}})");
    const std::string giveable_payload = dmcp::test::tool_text_payload(giveable);
    REQUIRE(body_has_text(giveable_payload, R"("catalog_scope":"giveable")"));
    REQUIRE(body_has_text(giveable_payload, "Shotgun"));
    REQUIRE(body_has_text(giveable_payload, "Shells"));
    REQUIRE_FALSE(body_has_text(giveable_payload, "Shotgun Shells"));
  }

  SECTION("direct command tool queues and completes through fake adapter") {
    const std::string session_id = dmcp::test::initialize_session(server.port);
    REQUIRE(!session_id.empty());

    const std::string queued =
        call_jsonrpc(server.port, session_id, 4, "tools/call",
                     R"({"name":"give_item","arguments":{"item_class":"Backpack","amount":1}})");
    const int64_t sequence = extract_sequence_from_tool_payload(queued);
    REQUIRE(sequence > 0);

    server.tick();

    std::string params = R"({"name":"get_command_result","arguments":{"sequence":)";
    params += std::to_string(sequence);
    params += "}}";

    const std::string result =
        call_jsonrpc(server.port, session_id, 5, "tools/call", params.c_str());
    const std::string payload = dmcp::test::tool_text_payload(result);
    REQUIRE(body_has_text(payload, R"("status":"success")"));
    REQUIRE(body_has_text(payload, R"("completed":true)"));
    REQUIRE(body_has_text(payload, R"("success":true)"));
  }

  SECTION("execute_batch uses direct tool-call shape") {
    const std::string session_id = dmcp::test::initialize_session(server.port);
    REQUIRE(!session_id.empty());

    const std::string queued = call_jsonrpc(
        server.port, session_id, 6, "tools/call",
        R"({"name":"execute_batch","arguments":{"calls":[{"name":"give_item","arguments":{"item_class":"Shotgun","amount":1}},{"name":"spawn_entity","arguments":{"entity_class":"DoomImp","x":320,"y":192,"angle":0}}]}})");
    const std::vector<int64_t> sequences = extract_sequences_from_batch_payload(queued);
    REQUIRE(sequences.size() == 2);
    REQUIRE(sequences[0] > 0);
    REQUIRE(sequences[1] > sequences[0]);

    server.tick();

    std::string params = R"({"name":"get_command_result","arguments":{"sequence":)";
    params += std::to_string(sequences[1]);
    params += "}}";

    const std::string result =
        call_jsonrpc(server.port, session_id, 7, "tools/call", params.c_str());
    const std::string payload = dmcp::test::tool_text_payload(result);
    REQUIRE(body_has_text(payload, R"("completed":true)"));
    REQUIRE(body_has_text(payload, R"("success":true)"));
  }

  SECTION("execute_batch rejects change_level entries") {
    const std::string session_id = dmcp::test::initialize_session(server.port);
    REQUIRE(!session_id.empty());

    const std::string queued = call_jsonrpc(
        server.port, session_id, 8, "tools/call",
        R"({"name":"execute_batch","arguments":{"calls":[{"name":"change_level","arguments":{"map_name":"MAP01"}}]}})");
    const std::string payload = dmcp::test::tool_text_payload(queued);
    REQUIRE(body_has_text(payload, R"("status":"error")"));
    REQUIRE(body_has_text(payload, "change_level is not supported"));
  }

  SECTION("execute_batch rejects ambiguous multi-spawn fields") {
    const std::string session_id = dmcp::test::initialize_session(server.port);
    REQUIRE(!session_id.empty());

    const std::string queued = call_jsonrpc(
        server.port, session_id, 9, "tools/call",
        R"({"name":"execute_batch","arguments":{"calls":[{"name":"spawn_entity","arguments":{"entity_class":"DoomImp","x":160,"y":96,"count":4}}]}})");
    const std::string payload = dmcp::test::tool_text_payload(queued);
    REQUIRE(body_has_text(payload, R"("status":"error")"));
    REQUIRE(body_has_text(payload, R"("queued":0)"));
    REQUIRE(body_has_text(payload, R"("rejected":1)"));
  }
}
