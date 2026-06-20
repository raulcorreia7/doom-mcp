#pragma once

#include <string>
#include <string_view>

#include "json/json.hpp"

namespace mcp {

std::string BuildJsonRpcRequest(int id, std::string_view method, std::string_view params_json = {});
std::string BuildJsonRpcNotification(std::string_view method, std::string_view params_json = {});
std::string BuildJsonRpcResult(std::string_view id_json, std::string_view result_json);
std::string BuildJsonRpcError(std::string_view id_json, int code, std::string_view message,
                              std::string_view data_json = {});

bool IsValidJson(std::string_view json);
bool IsJsonRpcResponseEnvelope(std::string_view json);
bool IsJsonRpcMessage(std::string_view json);
bool IsValidJsonRpcId(const json::Value& id_value);

std::string FormatSseMessage(std::string_view json_payload);

}  // namespace mcp
