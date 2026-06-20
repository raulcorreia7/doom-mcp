#include "doom/commands/json_parsers.hpp"

#include <cctype>
#include <cmath>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
#include <limits>
#include <string>
#include <string_view>

#include "dmcp/doom/api.h"

namespace dmcp {

bool parse_json_number(const json_value& val, double* out) {
  if (!out || !val || !val.is_number()) {
    return false;
  }

  const std::string text = val.dump();
  if (text.empty()) {
    return false;
  }

  char* end_ptr       = nullptr;
  errno               = 0;
  const double parsed = std::strtod(text.c_str(), &end_ptr);
  if (!end_ptr || end_ptr == text.c_str() || *end_ptr != '\0' || errno == ERANGE ||
      !std::isfinite(parsed)) {
    return false;
  }

  *out = parsed;
  return true;
}

bool parse_json_integer(const json_value& val, std::int64_t* out) {
  if (!out) {
    return false;
  }

  double parsed = 0.0;
  if (!parse_json_number(val, &parsed)) {
    return false;
  }
  if (std::floor(parsed) != parsed) {
    return false;
  }
  if (parsed < static_cast<double>(std::numeric_limits<std::int64_t>::min()) ||
      parsed > static_cast<double>(std::numeric_limits<std::int64_t>::max())) {
    return false;
  }

  *out = static_cast<std::int64_t>(parsed);
  return true;
}

bool parse_json_bool(const json_value& val, bool* out) {
  if (!out || !val) {
    return false;
  }

  if (!val.is_bool()) {
    return false;
  }

  *out = val.get_bool();
  return true;
}

json_value first_present_field(const json_value& obj, std::initializer_list<const char*> keys) {
  for (const char* key : keys) {
    if (obj.has_member(key)) {
      return obj[key];
    }
  }
  return {};
}

bool has_only_fields(const json_value& obj, std::initializer_list<const char*> allowed_keys) {
  if (!obj.is_object()) {
    return false;
  }

  for (const auto& member : obj.members()) {
    bool allowed = false;
    for (const char* key : allowed_keys) {
      if (member.first == key) {
        allowed = true;
        break;
      }
    }
    if (!allowed) {
      return false;
    }
  }

  return true;
}

bool copy_checked_string(char* dst, size_t dst_size, std::string_view value) {
  if (!dst || dst_size == 0 || value.empty() || value.size() >= dst_size) {
    return false;
  }
  mcp_strcpy_safe(dst, dst_size, value.data());
  return true;
}

bool copy_normalized_map_name(char* dst, size_t dst_size, std::string_view map_name) {
  if (!dst || dst_size == 0 || map_name.empty() || map_name.size() >= dst_size) {
    return false;
  }
  if (!is_valid_map_name(map_name)) {
    return false;
  }
  for (size_t i = 0; i < map_name.size(); ++i) {
    dst[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(map_name[i])));
  }
  dst[map_name.size()] = '\0';
  return true;
}

bool is_valid_map_name(std::string_view map_name) {
  if (map_name.size() == 4 && (map_name[0] == 'E' || map_name[0] == 'e') &&
      std::isdigit(static_cast<unsigned char>(map_name[1])) &&
      (map_name[2] == 'M' || map_name[2] == 'm') &&
      std::isdigit(static_cast<unsigned char>(map_name[3]))) {
    return true;
  }

  return map_name.size() == 5 && (map_name[0] == 'M' || map_name[0] == 'm') &&
         (map_name[1] == 'A' || map_name[1] == 'a') && (map_name[2] == 'P' || map_name[2] == 'p') &&
         std::isdigit(static_cast<unsigned char>(map_name[3])) &&
         std::isdigit(static_cast<unsigned char>(map_name[4]));
}

bool read_required_string(const json_value& obj, std::initializer_list<const char*> keys,
                          std::string_view* out) {
  if (!out) {
    return false;
  }

  const json_value candidate = first_present_field(obj, keys);
  if (!candidate || !candidate.is_string()) {
    return false;
  }

  const std::string_view value = candidate.get_string();
  if (value.empty()) {
    return false;
  }

  *out = value;
  return true;
}

bool read_optional_string(const json_value& obj, std::initializer_list<const char*> keys,
                          std::string_view default_value, std::string_view* out) {
  if (!out) {
    return false;
  }

  const json_value candidate = first_present_field(obj, keys);
  if (!candidate) {
    *out = default_value;
    return true;
  }
  if (!candidate.is_string()) {
    return false;
  }

  const std::string_view value = candidate.get_string();
  if (value.empty()) {
    return false;
  }

  *out = value;
  return true;
}

bool read_required_number(const json_value& obj, std::initializer_list<const char*> keys,
                          double* out) {
  const json_value candidate = first_present_field(obj, keys);
  return parse_json_number(candidate, out);
}

bool read_optional_number(const json_value& obj, std::initializer_list<const char*> keys,
                          double default_value, double* out) {
  if (!out) {
    return false;
  }

  const json_value candidate = first_present_field(obj, keys);
  if (!candidate) {
    *out = default_value;
    return true;
  }
  return parse_json_number(candidate, out);
}

bool read_required_int(const json_value& obj, std::initializer_list<const char*> keys,
                       std::int64_t* out) {
  const json_value candidate = first_present_field(obj, keys);
  return parse_json_integer(candidate, out);
}

bool read_optional_int(const json_value& obj, std::initializer_list<const char*> keys,
                       std::int64_t default_value, std::int64_t* out) {
  if (!out) {
    return false;
  }

  const json_value candidate = first_present_field(obj, keys);
  if (!candidate) {
    *out = default_value;
    return true;
  }
  return parse_json_integer(candidate, out);
}

bool read_required_bool(const json_value& obj, std::initializer_list<const char*> keys, bool* out) {
  const json_value candidate = first_present_field(obj, keys);
  return parse_json_bool(candidate, out);
}

bool read_optional_bool(const json_value& obj, std::initializer_list<const char*> keys,
                        bool default_value, bool* out) {
  if (!out) {
    return false;
  }

  const json_value candidate = first_present_field(obj, keys);
  if (!candidate) {
    *out = default_value;
    return true;
  }
  return parse_json_bool(candidate, out);
}

bool parse_position_coords(const json_value& params, position_coords* out) {
  if (!out) {
    return false;
  }

  if (params.has_member("position")) {
    return false;
  }

  double x = 0.0;
  double y = 0.0;
  if (!read_required_number(params, {"x"}, &x) || !read_required_number(params, {"y"}, &y) ||
      !std::isfinite(x) || !std::isfinite(y)) {
    return false;
  }

  double angle = 0.0;
  if (!read_optional_number(params, {"angle"}, 0.0, &angle) || !std::isfinite(angle)) {
    return false;
  }

  out->x     = x;
  out->y     = y;
  out->angle = angle;
  return true;
}

}  // namespace dmcp
