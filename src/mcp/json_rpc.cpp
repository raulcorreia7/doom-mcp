#include "mcp/json_rpc.hpp"

#include "mcp/generic/constants.h"

#include <string>

namespace mcp {

namespace {

using json::Document;
using json::Value;

bool ParseJsonObject(std::string_view json, Document* out_doc) {
  if (!out_doc) {
    return false;
  }

  if (!out_doc->parse(json)) {
    return false;
  }

  return out_doc->root().is_object();
}

void SetJsonRpcId(Value response_root, std::string_view id_json) {
  Document    id_doc;
  std::string id_envelope = "{\"id\":";
  id_envelope += id_json.empty() ? "null" : std::string(id_json);
  id_envelope += "}";

  if (ParseJsonObject(id_envelope, &id_doc)) {
    response_root.set_member("id", id_doc.root()["id"]);
    return;
  }

  Document null_id_doc;
  null_id_doc.parse("{\"id\":null}");
  response_root.set_member("id", null_id_doc.root()["id"]);
}

}  // namespace

std::string BuildJsonRpcRequest(int id, std::string_view method, std::string_view params_json) {
  Document request_doc;
  request_doc.create_object();

  Value root = request_doc.root();
  root.set_member("jsonrpc", MCP_JSONRPC_VERSION);
  root.set_member("id", static_cast<int64_t>(id));
  root.set_member("method", method);

  if (!params_json.empty()) {
    Document params_doc;
    if (params_doc.parse(std::string(params_json))) {
      root.set_member("params", params_doc.root());
    }
  }

  return request_doc.dump(false);
}

std::string BuildJsonRpcNotification(std::string_view method, std::string_view params_json) {
  Document notif_doc;
  notif_doc.create_object();

  Value root = notif_doc.root();
  root.set_member("jsonrpc", MCP_JSONRPC_VERSION);
  root.set_member("method", method);

  if (!params_json.empty()) {
    Document params_doc;
    if (params_doc.parse(std::string(params_json))) {
      root.set_member("params", params_doc.root());
    }
  }

  return notif_doc.dump(false);
}

std::string BuildJsonRpcResult(std::string_view id_json, std::string_view result_json) {
  Document response_doc;
  response_doc.create_object();

  Value response_root = response_doc.root();
  response_root.set_member("jsonrpc", MCP_JSONRPC_VERSION);
  SetJsonRpcId(response_root, id_json);

  Document    result_doc;
  std::string result_part = result_json.empty() ? "{}" : std::string(result_json);
  if (result_doc.parse(result_part)) {
    response_root.set_member("result", result_doc.root());
  } else {
    Document empty;
    empty.create_object();
    response_root.set_member("result", empty.root());
  }

  return response_doc.dump(false);
}

std::string BuildJsonRpcError(std::string_view id_json, int code, std::string_view message,
                              std::string_view data_json) {
  Document response_doc;
  response_doc.create_object();

  Value response_root = response_doc.root();
  response_root.set_member("jsonrpc", MCP_JSONRPC_VERSION);
  SetJsonRpcId(response_root, id_json);

  Document error_doc;
  error_doc.create_object();
  Value error_root = error_doc.root();
  error_root.set_member("code", static_cast<int64_t>(code));
  error_root.set_member("message", message);

  if (!data_json.empty()) {
    Document data_doc;
    if (data_doc.parse(std::string(data_json))) {
      error_root.set_member("data", data_doc.root());
    }
  }

  response_root.set_member("error", error_root);
  return response_doc.dump(false);
}

bool IsValidJson(std::string_view json) {
  Document doc;
  return doc.parse(json);
}

bool IsJsonRpcResponseEnvelope(std::string_view json) {
  Document doc;
  if (!doc.parse(json)) {
    return false;
  }

  Value root = doc.root();
  if (!root.is_object()) {
    return false;
  }

  if (std::string(root["jsonrpc"].get_string()) != MCP_JSONRPC_VERSION) {
    return false;
  }

  const bool has_result = root.has_member("result");
  const bool has_error  = root.has_member("error");
  return has_result != has_error;
}

bool IsJsonRpcMessage(std::string_view json) {
  Document doc;
  if (!doc.parse(json)) {
    return false;
  }

  Value root = doc.root();
  if (!root.is_object()) {
    return false;
  }

  if (std::string(root["jsonrpc"].get_string()) != MCP_JSONRPC_VERSION) {
    return false;
  }

  if (root.has_member("method") && root["method"].is_string()) {
    return true;
  }

  const bool has_result = root.has_member("result");
  const bool has_error  = root.has_member("error");
  return has_result != has_error;
}

bool IsValidJsonRpcId(const json::Value& id_value) {
  if (!id_value) {
    return false;
  }
  if (id_value.is_string() || id_value.is_null()) {
    return true;
  }
  if (!id_value.is_number()) {
    return false;
  }

  const std::string id_text = id_value.dump();
  return id_text.find('.') == std::string::npos && id_text.find('e') == std::string::npos &&
         id_text.find('E') == std::string::npos;
}

std::string FormatSseMessage(std::string_view json_payload) {
  std::string message = "event: message\n";

  if (json_payload.empty()) {
    message += "data: {}\n\n";
    return message;
  }

  std::string payload(json_payload);
  {
    Document payload_doc;
    if (payload_doc.parse(payload)) {
      payload = payload_doc.dump(true);
    }
  }

  size_t start = 0;
  while (start <= payload.size()) {
    size_t end = payload.find('\n', start);
    if (end == std::string::npos) {
      end = payload.size();
    }

    message += "data: ";
    message.append(payload, start, end - start);
    message += "\n";

    if (end == payload.size()) {
      break;
    }
    start = end + 1;
  }

  message += "\n";
  return message;
}

}  // namespace mcp
