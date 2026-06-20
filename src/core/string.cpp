#include "mcp/core/string.h"

namespace {

unsigned char lower_ascii(unsigned char value) {
  if (value >= static_cast<unsigned char>('A') && value <= static_cast<unsigned char>('Z')) {
    return static_cast<unsigned char>(value - static_cast<unsigned char>('A') +
                                      static_cast<unsigned char>('a'));
  }
  return value;
}

}  // namespace

extern "C" {

int mcp_strcmp_ci(const char* lhs, const char* rhs) {
  if (lhs == rhs) {
    return 0;
  }
  if (!lhs) {
    return -1;
  }
  if (!rhs) {
    return 1;
  }

  while (*lhs != '\0' && *rhs != '\0') {
    const unsigned char lhs_ch = lower_ascii(static_cast<unsigned char>(*lhs));
    const unsigned char rhs_ch = lower_ascii(static_cast<unsigned char>(*rhs));
    if (lhs_ch != rhs_ch) {
      return static_cast<int>(lhs_ch) - static_cast<int>(rhs_ch);
    }
    ++lhs;
    ++rhs;
  }

  return static_cast<int>(lower_ascii(static_cast<unsigned char>(*lhs))) -
         static_cast<int>(lower_ascii(static_cast<unsigned char>(*rhs)));
}

}  // extern "C"
