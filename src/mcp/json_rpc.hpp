#pragma once

#include <string>
#include <string_view>

#include "json/json.hpp"

namespace mcp {

std::string build_json_rpc_request(int id, std::string_view method,
                                   std::string_view params_json = {});
std::string build_json_rpc_notification(std::string_view method, std::string_view params_json = {});
std::string build_json_rpc_result(std::string_view id_json, std::string_view result_json);
std::string build_json_rpc_error(std::string_view id_json, int code, std::string_view message,
                                 std::string_view data_json = {});

bool is_valid_json(std::string_view json);
bool is_json_rpc_response_envelope(std::string_view json);
bool is_json_rpc_message(std::string_view json);
bool is_valid_json_rpc_id(const json::Value& id_value);

std::string format_sse_message(std::string_view json_payload);

}  // namespace mcp
