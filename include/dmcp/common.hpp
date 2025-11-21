#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <string_view>

namespace dmcp {

template <size_t N>
inline void CopyString(const char* source, std::array<char, N>& target) {
  if (!source) {
    target[0] = '\0';
    return;
  }
  std::string_view view{source};
  const size_t     copy_len = std::min<std::size_t>(view.size(), N - 1);
  std::copy_n(view.begin(), copy_len, target.begin());
  target[copy_len] = '\0';
}

}  // namespace dmcp
