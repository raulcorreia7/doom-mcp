#pragma once

#include "mcp/json/json.hpp"

#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
#include <limits>
#include <string>
#include <string_view>

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

// Reader helpers for command parsing
bool read_required_string(const json_value& obj, std::initializer_list<const char*> keys,
                          std::string_view* out);
bool read_optional_string(const json_value& obj, std::initializer_list<const char*> keys,
                          std::string_view default_value, std::string_view* out);
bool read_required_number(const json_value& obj, std::initializer_list<const char*> keys,
                          double* out);
bool read_optional_number(const json_value& obj, std::initializer_list<const char*> keys,
                          double default_value, double* out);
bool read_required_int(const json_value& obj, std::initializer_list<const char*> keys,
                       std::int64_t* out);
bool read_optional_int(const json_value& obj, std::initializer_list<const char*> keys,
                       std::int64_t default_value, std::int64_t* out);
bool read_required_bool(const json_value& obj, std::initializer_list<const char*> keys, bool* out);
bool read_optional_bool(const json_value& obj, std::initializer_list<const char*> keys,
                        bool default_value, bool* out);

}  // namespace dmcp
