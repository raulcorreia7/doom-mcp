#pragma once

#include <string>
#include <string_view>
#include "dmcp/doom/api.h"
#include "doom/internal/context.hpp"
#include "doom/internal/json_types.hpp"

namespace dmcp {

// ============================================================================
// Command tool definitions
// ============================================================================

struct command_tool_definition {
  const char* tool_name;
  const char* command_type;
  const char* description;
};

const command_tool_definition* find_command_tool(std::string_view tool_name);
const command_tool_definition* get_command_tools_array();
size_t                         get_command_tools_count();

// ============================================================================
// Shared Helper Functions
// ============================================================================

// Logging
void dmcp_log(const context* ctx, int level, const char* fmt, ...);

// Response building
std::string build_content_response(std::string_view text, bool is_error = false);
bool write_json_response(std::string_view payload, char* response_buffer, size_t response_size);
bool write_route_response(std::string_view json, int status, char* response_buffer,
                          size_t response_size, int* http_status);
std::string build_route_error(std::string_view code, std::string_view message);

// Game state
std::string get_game_state_json(context* ctx);

// Argument extraction
json_value  extract_tool_arguments(const json_value& params);
std::string extract_command_json(const json_value& params);
bool        queue_command_and_respond(context* ctx, std::string_view command_json,
                                      std::string_view command_name, char* response_buffer,
                                      size_t response_size);
bool queue_command_from_json(context* ctx, std::string_view command_json, dmcp_command_t* out_cmd,
                             std::string* error_message);
const char* resolve_command_type_for_method(std::string_view method_name);

// Sequence parsing
bool parse_sequence_field(const json_value& value, uint64_t* out_sequence);
bool parse_sequence_from_params(const char* request_json, uint64_t* out_sequence,
                                std::string* out_error);

// Command result building
std::string build_command_result_json(const dmcp_command_result_t& result);

// Schema helpers
void add_empty_object_schema(json_builder* schema);
void add_property(json_builder* props, const char* key, const char* type, const char* description);
void add_command_tool_schema(const char* tool_name, json_builder* schema);
void add_command_tool(json_builder* tools, const char* name, const char* description,
                      json_builder schema);

// ============================================================================
// Tool Handlers
// ============================================================================

bool handle_tool_get_game_state(context* ctx, char* response_buffer, size_t response_size);
bool handle_tool_get_screenshot(context* ctx, char* response_buffer, size_t response_size);
bool handle_tool_execute_command(context* ctx, const json_value& params, char* response_buffer,
                                 size_t response_size);
bool handle_tool_get_command_result(context* ctx, const json_value& params, char* response_buffer,
                                    size_t response_size);
bool handle_tool_get_available_content(context* ctx, const json_value& params,
                                       char* response_buffer, size_t response_size);
bool handle_tool_execute_batch(context* ctx, const json_value& params, char* response_buffer,
                               size_t response_size);
bool handle_tool_get_command_examples(context* ctx, char* response_buffer, size_t response_size);
bool handle_tool_command_alias(context* ctx, const command_tool_definition* command_tool,
                               const json_value& params, char* response_buffer,
                               size_t response_size);

// ============================================================================
// Schema Builders
// ============================================================================

json_builder build_get_game_state_schema();
json_builder build_get_screenshot_schema();
json_builder build_execute_command_schema();
json_builder build_get_command_result_schema();
json_builder build_get_available_content_schema();
json_builder build_execute_batch_schema();
json_builder build_get_command_examples_schema();

// ============================================================================
// Tools List/Call Handlers (MCP protocol)
// ============================================================================

bool handle_tools_list(void* user_data, const char* method, const char* request_json,
                       char* response_buffer, size_t response_size);
bool handle_tools_call(void* user_data, const char* method, const char* request_json,
                       char* response_buffer, size_t response_size);

}  // namespace dmcp
