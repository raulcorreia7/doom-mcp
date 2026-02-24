#pragma once

#include "doom/internal/json_types.hpp"

#include <algorithm>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
#include <limits>
#include <string>
#include <string_view>

namespace dmcp {

struct position_coords {
  double x;
  double y;
  double angle;
};

bool parse_json_number(const json_value& val, double* out);
bool parse_json_integer(const json_value& val, std::int64_t* out);
bool parse_json_bool(const json_value& val, bool* out);

json_value first_present_field(const json_value& obj, std::initializer_list<const char*> keys);

bool copy_checked_string(char* dst, size_t dst_size, std::string_view value);

bool copy_normalized_map_name(char* dst, size_t dst_size, std::string_view map_name);

bool is_valid_map_name(std::string_view map_name);

bool parse_position_coords(const json_value& params, position_coords* out);

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
