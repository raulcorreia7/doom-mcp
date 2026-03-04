#pragma once

#include <string>

namespace mcp::request_context {

struct request_metadata {
  std::string session_id;
  std::string protocol_version;
  std::string origin;
  std::string accept;
  std::string content_type;
};

struct response_metadata {
  std::string session_id;
  std::string protocol_version;
};

void set_current(const request_metadata& metadata);
void clear_current();

const request_metadata& current();

void set_response_session_id(const std::string& session_id);
void set_response_protocol_version(const std::string& protocol_version);

const response_metadata& response();
void                     clear_response();

}  // namespace mcp::request_context
