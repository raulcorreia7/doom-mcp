#pragma once

#include "mcp/json/json.hpp"

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
#include <limits>
#include <string>

namespace dmcp {

using json_value = ::mcp::json::Value;

// JSON parsing utilities
bool parse_json_number(const json_value& val, double* out);
bool parse_json_integer(const json_value& val, std::int64_t* out);
bool parse_json_bool(const json_value& val, bool* out);

// Get first present field from a list of keys
json_value first_present_field(const json_value& obj, std::initializer_list<const char*> keys);

// String copying helper
bool copy_checked_string(char* dst, size_t dst_size, std::string_view value);

// Validation functions
bool is_valid_map_name(std::string_view map_name);

}  // namespace dmcp
