#include "mcp/request_context.hpp"

namespace mcp::request_context {

namespace {
thread_local request_metadata  g_request_metadata;
thread_local response_metadata g_response_metadata;
}

void set_current(const request_metadata& metadata) { g_request_metadata = metadata; }

void clear_current() { g_request_metadata = request_metadata{}; }

const request_metadata& current() { return g_request_metadata; }

void set_response_session_id(const std::string& session_id) {
  g_response_metadata.session_id = session_id;
}

void set_response_protocol_version(const std::string& protocol_version) {
  g_response_metadata.protocol_version = protocol_version;
}

const response_metadata& response() { return g_response_metadata; }

void clear_response() { g_response_metadata = response_metadata{}; }

}  // namespace mcp::request_context
