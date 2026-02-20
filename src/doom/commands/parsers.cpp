#include "doom/commands/parsers.hpp"

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

  if (val.is_bool()) {
    *out = val.get_bool();
    return true;
  }

  if (!val.is_string()) {
    return false;
  }

  const std::string_view text = val.get_string();
  if (text == "true" || text == "True" || text == "TRUE" || text == "1" || text == "yes" ||
      text == "Yes" || text == "on") {
    *out = true;
    return true;
  }
  if (text == "false" || text == "False" || text == "FALSE" || text == "0" || text == "no" ||
      text == "No" || text == "off") {
    *out = false;
    return true;
  }

  return false;
}

json_value first_present_field(const json_value& obj, std::initializer_list<const char*> keys) {
  for (const char* key : keys) {
    if (obj.has_member(key)) {
      return obj[key];
    }
  }
  return {};
}

bool copy_checked_string(char* dst, size_t dst_size, std::string_view value) {
  if (!dst || dst_size == 0 || value.empty() || value.size() >= dst_size) {
    return false;
  }
  dmcp_strcpy(dst, value.data(), dst_size);
  return true;
}

bool is_valid_map_name(std::string_view map_name) {
  if (map_name.size() == 4 && map_name[0] == 'E' && map_name[2] == 'M' && map_name[1] >= '1' &&
      map_name[1] <= '9' && map_name[3] >= '1' && map_name[3] <= '9') {
    return true;
  }

  if (map_name.size() == 5 && map_name[0] == 'M' && map_name[1] == 'A' && map_name[2] == 'P' &&
      map_name[3] >= '0' && map_name[3] <= '9' && map_name[4] >= '0' && map_name[4] <= '9') {
    return true;
  }

  return false;
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

}  // namespace dmcp
