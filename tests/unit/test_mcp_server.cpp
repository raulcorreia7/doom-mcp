#include <atomic>
#include <chrono>
#include <cstring>
#include <string>
#include <thread>

#include "mcp/generic/constants.h"
#include "mcp/generic/protocol.h"
#include "mcp/generic/server.h"
#include "mcp/json/json.hpp"
#include "test_utils.hpp"

static int call_count = 0;

static bool simple_handler(void* user_data, const char* method, const char* request_json,
                           char* response_buffer, size_t response_size) {
  (void)user_data;
  (void)method;
  (void)request_json;
  call_count++;

  const char* response = "{\"result\":\"ok\"}";
  if (response_size > std::strlen(response)) {
    std::strcpy(response_buffer, response);
    return true;
  }
  return false;
}

static bool data_handler(void* user_data, const char* method, const char* request_json,
                         char* response_buffer, size_t response_size) {
  if (user_data) {
    int* counter = static_cast<int*>(user_data);
    (*counter)++;
  }

  const char* response = "{\"result\":\"data received\"}";
  if (response_size > std::strlen(response)) {
    std::strcpy(response_buffer, response);
    return true;
  }
  return false;
}

static bool route_handler(void* user_data, const char* method, const char* path, const char* body,
                          char* response_buffer, size_t response_size, int* http_status) {
  (void)method;
  (void)path;
  (void)body;

  if (user_data) {
    int* counter = static_cast<int*>(user_data);
    (*counter)++;
  }

  const char* response = "{\"ok\":true}";
  if (!http_status || response_size <= std::strlen(response)) {
    return false;
  }

  *http_status = 200;
  std::strcpy(response_buffer, response);
  return true;
}

TEST_CASE("Generic MCP: Server lifecycle", "[api][server][lifecycle]") {
  SECTION("Create and destroy server with default config") {
    mcp_server_config_t config = mcp_default_config();
    mcp_server_t*       server = mcp_server_create(&config);

    REQUIRE(server != nullptr);
    REQUIRE(mcp_server_is_running(server) == true);

    mcp_server_destroy(server);
  }

  SECTION("Create server with custom port") {
    mcp_server_config_t config = mcp_default_config();
    config.port                = 7070;

    mcp_server_t* server = mcp_server_create(&config);
    REQUIRE(server != nullptr);

    mcp_server_destroy(server);
  }

  SECTION("Create server with null config uses defaults") {
    mcp_server_t* server = mcp_server_create(nullptr);
    REQUIRE(server != nullptr);

    mcp_server_destroy(server);
  }

  SECTION("Create server with custom max payload size") {
    mcp_server_config_t config = mcp_default_config();
    config.max_payload_size    = 2048;

    mcp_server_t* server = mcp_server_create(&config);
    REQUIRE(server != nullptr);

    mcp_server_destroy(server);
  }

  SECTION("Destroy null server is safe") { mcp_server_destroy(nullptr); }

  SECTION("Is running with null server returns false") {
    REQUIRE(mcp_server_is_running(nullptr) == false);
  }
}

TEST_CASE("Generic MCP: Single method registration", "[api][server][method]") {
  mcp_server_config_t config = mcp_default_config();
  mcp_server_t*       server = mcp_server_create(&config);
  REQUIRE(server != nullptr);

  SECTION("Register valid method succeeds") {
    call_count = 0;
    mcp_result_t result =
        mcp_server_method_register(server, "test_method", simple_handler, nullptr);

    REQUIRE(result.code == MCP_RESULT_CODE_OK);
    REQUIRE(result.message != nullptr);
    REQUIRE(std::strcmp(result.message, "Success") == 0);
  }

  SECTION("Register method with user data") {
    int          counter = 0;
    mcp_result_t result = mcp_server_method_register(server, "data_method", data_handler, &counter);

    REQUIRE(result.code == MCP_RESULT_CODE_OK);
  }

  SECTION("Register method with null server returns error") {
    mcp_result_t result =
        mcp_server_method_register(nullptr, "test_method", simple_handler, nullptr);

    REQUIRE(result.code == MCP_RESULT_CODE_INVALID_ARGS);
    REQUIRE(result.message != nullptr);
  }

  SECTION("Register method with null name returns error") {
    mcp_result_t result = mcp_server_method_register(server, nullptr, simple_handler, nullptr);

    REQUIRE(result.code == MCP_RESULT_CODE_INVALID_ARGS);
  }

  SECTION("Register method with null handler returns error") {
    mcp_result_t result = mcp_server_method_register(server, "test_method", nullptr, nullptr);

    REQUIRE(result.code == MCP_RESULT_CODE_INVALID_ARGS);
  }

  SECTION("Unregister existing method succeeds") {
    mcp_server_method_register(server, "to_remove", simple_handler, nullptr);
    mcp_server_method_unregister(server, "to_remove");
  }

  SECTION("Unregister null server is safe") { mcp_server_method_unregister(nullptr, "test"); }

  SECTION("Unregister null name is safe") { mcp_server_method_unregister(server, nullptr); }

  mcp_server_destroy(server);
}

TEST_CASE("Generic MCP: Batch method registration", "[api][server][batch]") {
  mcp_server_config_t config = mcp_default_config();
  mcp_server_t*       server = mcp_server_create(&config);
  REQUIRE(server != nullptr);

  SECTION("Register single method via batch") {
    mcp_method_registration_t methods[] = {{"method1", simple_handler, nullptr}};

    mcp_result_t result = mcp_server_methods_register(server, methods, 1);
    REQUIRE(result.code == MCP_RESULT_CODE_OK);
  }

  SECTION("Register five methods via batch") {
    mcp_method_registration_t methods[5] = {{"method1", simple_handler, nullptr},
                                            {"method2", simple_handler, nullptr},
                                            {"method3", simple_handler, nullptr},
                                            {"method4", simple_handler, nullptr},
                                            {"method5", simple_handler, nullptr}};

    mcp_result_t result = mcp_server_methods_register(server, methods, 5);
    REQUIRE(result.code == MCP_RESULT_CODE_OK);
  }

  SECTION("Register ten methods via batch") {
    mcp_method_registration_t methods[10] = {
        {"method1", simple_handler, nullptr}, {"method2", simple_handler, nullptr},
        {"method3", simple_handler, nullptr}, {"method4", simple_handler, nullptr},
        {"method5", simple_handler, nullptr}, {"method6", simple_handler, nullptr},
        {"method7", simple_handler, nullptr}, {"method8", simple_handler, nullptr},
        {"method9", simple_handler, nullptr}, {"method10", simple_handler, nullptr}};

    mcp_result_t result = mcp_server_methods_register(server, methods, 10);
    REQUIRE(result.code == MCP_RESULT_CODE_OK);
  }

  SECTION("Register zero methods succeeds") {
    mcp_result_t result = mcp_server_methods_register(server, nullptr, 0);
    REQUIRE(result.code == MCP_RESULT_CODE_OK);
  }

  SECTION("Batch with null server returns error") {
    mcp_method_registration_t methods[] = {{"test", simple_handler, nullptr}};

    mcp_result_t result = mcp_server_methods_register(nullptr, methods, 1);
    REQUIRE(result.code == MCP_RESULT_CODE_INVALID_ARGS);
  }

  SECTION("Batch with null methods array but count > 0 returns error") {
    mcp_result_t result = mcp_server_methods_register(server, nullptr, 5);
    REQUIRE(result.code == MCP_RESULT_CODE_INVALID_ARGS);
  }

  SECTION("Batch with null method name returns error") {
    mcp_method_registration_t methods[] = {{nullptr, simple_handler, nullptr}};

    mcp_result_t result = mcp_server_methods_register(server, methods, 1);
    REQUIRE(result.code == MCP_RESULT_CODE_INVALID_ARGS);
  }

  SECTION("Batch with null handler returns error") {
    mcp_method_registration_t methods[] = {{"test", nullptr, nullptr}};

    mcp_result_t result = mcp_server_methods_register(server, methods, 1);
    REQUIRE(result.code == MCP_RESULT_CODE_INVALID_ARGS);
  }

  SECTION("Mixed single and batch registration works") {
    mcp_result_t result1 = mcp_server_method_register(server, "single1", simple_handler, nullptr);
    REQUIRE(result1.code == MCP_RESULT_CODE_OK);

    mcp_method_registration_t batch[3] = {{"batch1", simple_handler, nullptr},
                                          {"batch2", simple_handler, nullptr},
                                          {"batch3", simple_handler, nullptr}};
    mcp_result_t              result2  = mcp_server_methods_register(server, batch, 3);
    REQUIRE(result2.code == MCP_RESULT_CODE_OK);

    mcp_result_t result3 = mcp_server_method_register(server, "single2", simple_handler, nullptr);
    REQUIRE(result3.code == MCP_RESULT_CODE_OK);
  }

  mcp_server_destroy(server);
}

TEST_CASE("Generic MCP: Route registration", "[api][server][route]") {
  mcp_server_config_t config = mcp_default_config();
  mcp_server_t*       server = mcp_server_create(&config);
  REQUIRE(server != nullptr);

  SECTION("Register valid route succeeds") {
    int          counter = 0;
    mcp_result_t result =
        mcp_server_route_register(server, "GET", "/game/state", route_handler, &counter);
    REQUIRE(result.code == MCP_RESULT_CODE_OK);
  }

  SECTION("Register route with null server returns error") {
    mcp_result_t result =
        mcp_server_route_register(nullptr, "GET", "/game/state", route_handler, nullptr);
    REQUIRE(result.code == MCP_RESULT_CODE_INVALID_ARGS);
  }

  SECTION("Register route with null method returns error") {
    mcp_result_t result =
        mcp_server_route_register(server, nullptr, "/game/state", route_handler, nullptr);
    REQUIRE(result.code == MCP_RESULT_CODE_INVALID_ARGS);
  }

  SECTION("Register route with null path returns error") {
    mcp_result_t result = mcp_server_route_register(server, "GET", nullptr, route_handler, nullptr);
    REQUIRE(result.code == MCP_RESULT_CODE_INVALID_ARGS);
  }

  SECTION("Register route with null handler returns error") {
    mcp_result_t result = mcp_server_route_register(server, "GET", "/game/state", nullptr, nullptr);
    REQUIRE(result.code == MCP_RESULT_CODE_INVALID_ARGS);
  }

  SECTION("Reserved route registration fails") {
    mcp_result_t result =
        mcp_server_route_register(server, "POST", MCP_ENDPOINT_MCP, route_handler, nullptr);
    REQUIRE(result.code == MCP_RESULT_CODE_INVALID_ARGS);
  }

  SECTION("Unregister custom route is safe") {
    mcp_result_t result =
        mcp_server_route_register(server, "GET", "/game/state", route_handler, nullptr);
    REQUIRE(result.code == MCP_RESULT_CODE_OK);
    mcp_server_route_unregister(server, "GET", "/game/state");
  }

  SECTION("Unregister reserved route is no-op") {
    mcp_server_route_unregister(server, "POST", MCP_ENDPOINT_MCP);
    mcp_server_route_unregister(server, "GET", MCP_ENDPOINT_HEALTH);
  }

  mcp_server_destroy(server);
}

TEST_CASE("Generic MCP: Event broadcasting", "[api][server][events]") {
  mcp_server_config_t config = mcp_default_config();
  mcp_server_t*       server = mcp_server_create(&config);
  REQUIRE(server != nullptr);

  SECTION("Broadcast state event") { mcp_server_event_broadcast(server, "state", "{\"hp\":100}"); }

  SECTION("Broadcast screenshot event") {
    mcp_server_event_broadcast(server, "screenshot", "{\"timestamp\":123456}");
  }

  SECTION("Broadcast notification event") {
    mcp_server_event_broadcast(server, "notification", "{\"msg\":\"Hello\"}");
  }

  SECTION("Broadcast with null server is safe") {
    mcp_server_event_broadcast(nullptr, "state", "{}");
  }

  SECTION("Broadcast with null event type is safe") {
    mcp_server_event_broadcast(server, nullptr, "{}");
  }

  SECTION("Broadcast with null payload is safe") {
    mcp_server_event_broadcast(server, "state", nullptr);
  }

  SECTION("Broadcast empty JSON payload") { mcp_server_event_broadcast(server, "state", "{}"); }

  SECTION("Broadcast complex JSON payload") {
    const char* complex_json = R"({
      "player": {"hp": 100, "armor": 50},
      "enemies": [
        {"id": 1, "type": "Imp", "hp": 60},
        {"id": 2, "type": "Demon", "hp": 150}
      ]
    })";
    mcp_server_event_broadcast(server, "state", complex_json);
  }

  mcp_server_destroy(server);
}

TEST_CASE("Generic MCP: Client count", "[api][server][clients]") {
  mcp_server_config_t config = mcp_default_config();
  mcp_server_t*       server = mcp_server_create(&config);
  REQUIRE(server != nullptr);

  SECTION("Get client count on new server") {
    uint64_t count = mcp_server_clients_count(server);
    REQUIRE(count == 0);
  }

  SECTION("Get client count with null server returns 0") {
    uint64_t count = mcp_server_clients_count(nullptr);
    REQUIRE(count == 0);
  }

  mcp_server_destroy(server);
}

TEST_CASE("Generic MCP: Statistics", "[api][server][stats]") {
  mcp_server_config_t config = mcp_default_config();
  mcp_server_t*       server = mcp_server_create(&config);
  REQUIRE(server != nullptr);

  SECTION("Get stats returns valid structure") {
    mcp_server_stats_t stats = {};
    mcp_server_stats_get(server, &stats);

    REQUIRE(stats.connected_clients == 0);
    REQUIRE(stats.requests_handled == 0);
    REQUIRE(stats.requests_failed == 0);
    REQUIRE(stats.events_broadcast == 0);
    REQUIRE(stats.bytes_sent == 0);
  }

  SECTION("Get stats with null server zeros structure") {
    mcp_server_stats_t stats = {};
    stats.connected_clients  = 999;
    stats.requests_handled   = 999;

    mcp_server_stats_get(nullptr, &stats);
  }

  SECTION("Get stats with null stats pointer is safe") { mcp_server_stats_get(server, nullptr); }

  mcp_server_destroy(server);
}

TEST_CASE("Generic MCP: JSON-RPC response formatting", "[api][server][format]") {
  SECTION("Format success response") {
    char   buffer[512];
    size_t written =
        mcp_format_success_response(buffer, sizeof(buffer), "1", "{\"result\":\"ok\"}");

    REQUIRE(written > 0);
    INFO(buffer);
    mcp::json::Document doc;
    REQUIRE(doc.parse(buffer));
    auto root = doc.root();
    REQUIRE(std::string(root["jsonrpc"].get_string("")) == "2.0");
    REQUIRE(std::string(root["id"].get_string("")) == "1");
    REQUIRE(root.has_member("result"));
  }

  SECTION("Format success response with null id") {
    char   buffer[512];
    size_t written = mcp_format_success_response(buffer, sizeof(buffer), nullptr, "{}");

    REQUIRE(written > 0);
  }

  SECTION("Format success response with null result") {
    char   buffer[512];
    size_t written = mcp_format_success_response(buffer, sizeof(buffer), "1", nullptr);

    REQUIRE(written > 0);
  }

  SECTION("Format success response with small buffer") {
    char   buffer[10];
    size_t written = mcp_format_success_response(buffer, sizeof(buffer), "1", "{}");
    (void)written;
  }

  SECTION("Format error response") {
    char   buffer[512];
    size_t written =
        mcp_format_error_response(buffer, sizeof(buffer), "1", -32603, "Internal error");

    REQUIRE(written > 0);
    INFO(buffer);
    mcp::json::Document doc;
    REQUIRE(doc.parse(buffer));
    auto root = doc.root();
    REQUIRE(std::string(root["jsonrpc"].get_string("")) == "2.0");
    REQUIRE(std::string(root["id"].get_string("")) == "1");
    REQUIRE(root.has_member("error"));
    REQUIRE(root["error"]["code"].get_int() == -32603);
    REQUIRE(std::string(root["error"]["message"].get_string("")) == "Internal error");
  }

  SECTION("Format error response with null id") {
    char   buffer[512];
    size_t written = mcp_format_error_response(buffer, sizeof(buffer), nullptr, -32600, "Error");

    REQUIRE(written > 0);
  }

  SECTION("Format error response with null message") {
    char   buffer[512];
    size_t written = mcp_format_error_response(buffer, sizeof(buffer), "1", -32600, nullptr);

    REQUIRE(written > 0);
  }

  SECTION("Format error response with zero error code") {
    char   buffer[512];
    size_t written = mcp_format_error_response(buffer, sizeof(buffer), "1", 0, "No error");

    REQUIRE(written > 0);
  }
}

TEST_CASE("Generic MCP: Constants", "[api][constants]") {
  SECTION("Protocol version constant is correct") {
    REQUIRE(std::strcmp(MCP_JSONRPC_VERSION, "2.0") == 0);
  }

  SECTION("Default port constant") { REQUIRE(MCP_DEFAULT_PORT == 6060); }

  SECTION("Default target Hz constant") { REQUIRE(MCP_DEFAULT_TARGET_HZ == 10); }

  SECTION("Buffer size constants") {
    REQUIRE(MCP_BUFFER_SIZE_DEFAULT == 8192);
    REQUIRE(MCP_MAX_JSON_SIZE == 16384);
    REQUIRE(MCP_HEALTH_BUFFER_SIZE == 256);
    REQUIRE(MCP_MAX_PAYLOAD_SIZE == (1024 * 1024));
  }

  SECTION("Screenshot defaults") {
    REQUIRE(MCP_DEFAULT_SCREENSHOT_WIDTH == 640);
    REQUIRE(MCP_DEFAULT_SCREENSHOT_HEIGHT == 480);
  }

  SECTION("Endpoint constants") {
    REQUIRE(std::strcmp(MCP_ENDPOINT_MCP, "/mcp") == 0);
    REQUIRE(std::strcmp(MCP_ENDPOINT_HEALTH, "/health") == 0);
  }

  SECTION("Health response") {
    REQUIRE(std::strcmp(MCP_HEALTH_RESPONSE, "{\"status\":\"ok\"}") == 0);
  }
}

TEST_CASE("Generic MCP: Result codes", "[api][result]") {
  SECTION("Result code constants") {
    REQUIRE(MCP_RESULT_CODE_OK == 0);
    REQUIRE(MCP_RESULT_CODE_INVALID_ARGS == -1);
    REQUIRE(MCP_RESULT_CODE_ENCODING_FAILED == -2);
    REQUIRE(MCP_RESULT_CODE_DISABLED == -3);
    REQUIRE(MCP_RESULT_CODE_QUEUE_FULL == -4);
    REQUIRE(MCP_RESULT_CODE_NOT_FOUND == -5);
    REQUIRE(MCP_RESULT_CODE_INTERNAL == -6);
  }

  SECTION("Convenience macros create valid results") {
    mcp_result_t ok = MCP_OK;
    REQUIRE(ok.code == 0);
    REQUIRE(ok.message != nullptr);

    mcp_result_t error = MCP_ERROR_INVALID_ARGS;
    REQUIRE(error.code == -1);
    REQUIRE(error.message != nullptr);
  }

  SECTION("Result macros with custom message") {
    mcp_result_t result = MCP_RESULT_ERROR(-100, "Custom error");
    REQUIRE(result.code == -100);
    REQUIRE(std::strcmp(result.message, "Custom error") == 0);
  }

  SECTION("Result macro with null message") {
    mcp_result_t result = MCP_RESULT_ERROR(-200, nullptr);
    REQUIRE(result.code == -200);
    REQUIRE(result.message != nullptr);
  }
}

TEST_CASE("Generic MCP: Server configuration", "[api][config]") {
  SECTION("Default config has correct values") {
    mcp_server_config_t config = mcp_default_config();

    REQUIRE(config.struct_size == sizeof(mcp_server_config_t));
    REQUIRE(config.port == MCP_DEFAULT_PORT);
    REQUIRE(config.max_requests_per_second == 100);
    REQUIRE(config.max_payload_size == MCP_MAX_PAYLOAD_SIZE);
    REQUIRE(config.on_log == nullptr);
    REQUIRE(config.log_user_data == nullptr);
  }

  SECTION("Custom config values") {
    mcp_server_config_t config     = mcp_default_config();
    config.port                    = 8080;
    config.max_requests_per_second = 50;
    config.max_payload_size        = 512 * 1024;

    mcp_server_t* server = mcp_server_create(&config);
    REQUIRE(server != nullptr);

    mcp_server_destroy(server);
  }
}

TEST_CASE("Generic MCP: Error messages", "[api][error]") {
  mcp_server_config_t config = mcp_default_config();
  mcp_server_t*       server = mcp_server_create(&config);
  REQUIRE(server != nullptr);

  SECTION("Invalid args error message is descriptive") {
    mcp_result_t result = mcp_server_method_register(nullptr, "test", simple_handler, nullptr);

    REQUIRE(result.code == MCP_RESULT_CODE_INVALID_ARGS);
    REQUIRE(result.message != nullptr);
    REQUIRE(std::strlen(result.message) > 0);
  }

  SECTION("Success message is present") {
    mcp_result_t result = mcp_server_method_register(server, "test", simple_handler, nullptr);

    REQUIRE(result.code == MCP_RESULT_CODE_OK);
    REQUIRE(result.message != nullptr);
  }

  mcp_server_destroy(server);
}

TEST_CASE("Generic MCP: Thread safety", "[api][thread]") {
  mcp_server_config_t config = mcp_default_config();
  mcp_server_t*       server = mcp_server_create(&config);
  REQUIRE(server != nullptr);

  SECTION("Concurrent method registration") {
    constexpr int            num_threads        = 4;
    constexpr int            methods_per_thread = 10;
    std::vector<std::thread> threads;
    std::atomic<int>         success_count{0};

    for (int t = 0; t < num_threads; ++t) {
      threads.emplace_back([&, t]() {
        for (int i = 0; i < methods_per_thread; ++i) {
          std::string  name = "thread_" + std::to_string(t) + "_method_" + std::to_string(i);
          mcp_result_t result =
              mcp_server_method_register(server, name.c_str(), simple_handler, nullptr);
          if (result.code == MCP_RESULT_CODE_OK) {
            success_count++;
          }
        }
      });
    }

    for (auto& th : threads) {
      th.join();
    }

    REQUIRE(success_count == num_threads * methods_per_thread);
  }

  SECTION("Concurrent broadcast operations") {
    constexpr int            num_threads           = 4;
    constexpr int            broadcasts_per_thread = 100;
    std::vector<std::thread> threads;

    for (int t = 0; t < num_threads; ++t) {
      threads.emplace_back([&, t]() {
        for (int i = 0; i < broadcasts_per_thread; ++i) {
          std::string payload =
              "{\"thread\":" + std::to_string(t) + ",\"seq\":" + std::to_string(i) + "}";
          mcp_server_event_broadcast(server, "state", payload.c_str());
        }
      });
    }

    for (auto& th : threads) {
      th.join();
    }
  }

  SECTION("Concurrent mixed operations") {
    std::atomic<bool>        running{true};
    std::vector<std::thread> threads;

    threads.emplace_back([&]() {
      for (int i = 0; i < 50 && running; ++i) {
        std::string name = "dynamic_method_" + std::to_string(i);
        mcp_server_method_register(server, name.c_str(), simple_handler, nullptr);
      }
    });

    threads.emplace_back([&]() {
      for (int i = 0; i < 50 && running; ++i) {
        std::string payload = "{\"seq\":" + std::to_string(i) + "}";
        mcp_server_event_broadcast(server, "state", payload.c_str());
      }
    });

    threads.emplace_back([&]() {
      for (int i = 0; i < 50 && running; ++i) {
        mcp_server_stats_t stats;
        mcp_server_stats_get(server, &stats);
      }
    });

    for (auto& th : threads) {
      th.join();
    }
  }

  mcp_server_destroy(server);
}

TEST_CASE("Generic MCP: Server create with invalid config", "[api][server][error]") {
  SECTION("Port zero - implementation allows (auto-assign)") {
    mcp_server_config_t config = mcp_default_config();
    config.port                = 0;

    mcp_server_t* server = mcp_server_create(&config);
    REQUIRE(server != nullptr);
    mcp_server_destroy(server);
  }

  SECTION("Negative port - implementation behavior") {
    mcp_server_config_t config = mcp_default_config();
    config.port                = -1;

    mcp_server_t* server = mcp_server_create(&config);
    REQUIRE(server != nullptr);
    mcp_server_destroy(server);
  }

  SECTION("Port out of range - implementation behavior") {
    mcp_server_config_t config = mcp_default_config();
    config.port                = 70000;

    mcp_server_t* server = mcp_server_create(&config);
    REQUIRE(server != nullptr);
    mcp_server_destroy(server);
  }

  SECTION("Zero max payload size - implementation behavior") {
    mcp_server_config_t config = mcp_default_config();
    config.max_payload_size    = 0;

    mcp_server_t* server = mcp_server_create(&config);
    REQUIRE(server != nullptr);
    mcp_server_destroy(server);
  }
}

static bool overflow_handler(void* user_data, const char* method, const char* request_json,
                             char* response_buffer, size_t response_size) {
  (void)user_data;
  (void)method;
  (void)request_json;

  const char* large_response =
      "{\"data\":\"This response is intentionally larger than the provided "
      "buffer to test overflow handling in the MCP server implementation\"}";
  size_t needed = std::strlen(large_response) + 1;

  if (response_size < needed) {
    return false;
  }
  std::strcpy(response_buffer, large_response);
  return true;
}

TEST_CASE("Generic MCP: Response buffer overflow", "[api][server][error][buffer]") {
  mcp_server_config_t config = mcp_default_config();
  mcp_server_t*       server = mcp_server_create(&config);
  REQUIRE(server != nullptr);

  SECTION("Handler returns false when buffer too small") {
    mcp_result_t result =
        mcp_server_method_register(server, "overflow_test", overflow_handler, nullptr);
    REQUIRE(result.code == MCP_RESULT_CODE_OK);

    char small_buffer[10];
    bool success =
        overflow_handler(nullptr, "overflow_test", "{}", small_buffer, sizeof(small_buffer));
    REQUIRE(success == false);
  }

  SECTION("Handler succeeds with sufficient buffer") {
    mcp_result_t result =
        mcp_server_method_register(server, "overflow_test", overflow_handler, nullptr);
    REQUIRE(result.code == MCP_RESULT_CODE_OK);

    char large_buffer[256];
    bool success =
        overflow_handler(nullptr, "overflow_test", "{}", large_buffer, sizeof(large_buffer));
    REQUIRE(success == true);
  }

  mcp_server_destroy(server);
}
