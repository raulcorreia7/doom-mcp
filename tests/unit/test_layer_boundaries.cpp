#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#ifndef DMCP_SOURCE_DIR
#define DMCP_SOURCE_DIR "."
#endif

namespace fs = std::filesystem;

static std::string read_file_content(const fs::path& path) {
  std::ifstream file(path);
  if (!file) {
    return "";
  }
  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

static std::vector<std::string> extract_includes(const std::string& content) {
  std::vector<std::string> includes;
  std::regex               include_pattern(R"(^\s*#\s*include\s*[<"]([^>"]+)[>"])");
  std::istringstream       stream(content);
  std::string              line;

  while (std::getline(stream, line)) {
    std::smatch match;
    if (std::regex_search(line, match, include_pattern)) {
      includes.push_back(match[1].str());
    }
  }

  return includes;
}

static bool contains_include(const std::vector<std::string>& includes, const std::string& pattern) {
  for (const auto& inc : includes) {
    if (inc.find(pattern) != std::string::npos) {
      return true;
    }
  }
  return false;
}

static bool is_cpp_file(const fs::path& path) {
  std::string ext = path.extension().string();
  std::transform(ext.begin(), ext.end(), ext.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  return ext == ".cpp" || ext == ".cc" || ext == ".cxx";
}

static bool is_header_file(const fs::path& path) {
  std::string ext = path.extension().string();
  std::transform(ext.begin(), ext.end(), ext.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  return ext == ".hpp" || ext == ".h" || ext == ".hxx";
}

static bool path_contains(const fs::path& path, const std::string& segment) {
  for (const auto& part : path) {
    if (part.string() == segment) {
      return true;
    }
  }
  return false;
}

static void require_no_includes_matching(const fs::path&                 root,
                                         const std::vector<std::string>& forbidden_patterns,
                                         const char*                     message) {
  std::vector<fs::path> violations;

  if (!fs::exists(root)) {
    return;
  }

  for (const auto& entry : fs::recursive_directory_iterator(root)) {
    if (!entry.is_regular_file()) {
      continue;
    }
    if (!is_cpp_file(entry.path()) && !is_header_file(entry.path())) {
      continue;
    }

    std::string content  = read_file_content(entry.path());
    auto        includes = extract_includes(content);

    for (const auto& pattern : forbidden_patterns) {
      if (contains_include(includes, pattern)) {
        violations.push_back(entry.path());
        break;
      }
    }
  }

  if (!violations.empty()) {
    std::string msg = message;
    msg += "\n";
    for (const auto& v : violations) {
      msg += "  " + v.string() + "\n";
    }
    FAIL_CHECK(msg);
  }
}

TEST_CASE("Layer boundaries: Doom core does not include mcp/json/json.hpp directly",
          "[layer][doom]") {
  fs::path              doom_src_dir = fs::path(DMCP_SOURCE_DIR) / "src" / "doom";
  std::vector<fs::path> violations;

  if (!fs::exists(doom_src_dir)) {
    FAIL_CHECK("Could not locate src/doom directory");
    return;
  }

  for (const auto& entry : fs::recursive_directory_iterator(doom_src_dir)) {
    if (!entry.is_regular_file()) {
      continue;
    }
    if (!is_cpp_file(entry.path()) && !is_header_file(entry.path())) {
      continue;
    }

    std::string filename = entry.path().filename().string();
    if (filename == "json_types.hpp") {
      continue;
    }

    std::string content  = read_file_content(entry.path());
    auto        includes = extract_includes(content);

    if (contains_include(includes, "mcp/json/json.hpp")) {
      violations.push_back(entry.path());
    }
  }

  if (!violations.empty()) {
    std::string msg =
        "Doom core files must use doom/internal/json_types.hpp, not mcp/json/json.hpp directly. "
        "Violations:\n";
    for (const auto& v : violations) {
      msg += "  " + v.string() + "\n";
    }
    FAIL_CHECK(msg);
  }
}

TEST_CASE("Layer boundaries: Core foundation has no generic, doom, or adapter dependencies",
          "[layer][core]") {
  require_no_includes_matching(
      fs::path(DMCP_SOURCE_DIR) / "src" / "core",
      {"mcp/generic/", "dmcp/doom/", "dmcp/adapter/", "dmcp/adapters/", "doom/"},
      "Core foundation files must not depend on generic MCP, Doom MCP, or adapters. Violations:");

  require_no_includes_matching(
      fs::path(DMCP_SOURCE_DIR) / "include" / "mcp" / "core",
      {"mcp/generic/", "dmcp/doom/", "dmcp/adapter/", "dmcp/adapters/", "doom/"},
      "Core foundation public headers must not depend on generic MCP, Doom MCP, or adapters. "
      "Violations:");
}

TEST_CASE("Layer boundaries: Doom core does not include adapter headers",
          "[layer][doom][adapter]") {
  fs::path              doom_src_dir = fs::path(DMCP_SOURCE_DIR) / "src" / "doom";
  std::vector<fs::path> violations;

  if (!fs::exists(doom_src_dir)) {
    FAIL_CHECK("Could not locate src/doom directory");
    return;
  }

  for (const auto& entry : fs::recursive_directory_iterator(doom_src_dir)) {
    if (!entry.is_regular_file()) {
      continue;
    }
    if (!is_cpp_file(entry.path()) && !is_header_file(entry.path())) {
      continue;
    }

    std::string content  = read_file_content(entry.path());
    auto        includes = extract_includes(content);

    if (contains_include(includes, "adapters/zdoom/") ||
        contains_include(includes, "adapters/crispy-doom/") ||
        contains_include(includes, "dmcp/adapter/") ||
        contains_include(includes, "dmcp/adapters/")) {
      violations.push_back(entry.path());
    }
  }

  if (!violations.empty()) {
    std::string msg = "Doom core files must not include adapter-scoped headers. Violations:\n";
    for (const auto& v : violations) {
      msg += "  " + v.string() + "\n";
    }
    FAIL_CHECK(msg);
  }
}

TEST_CASE("Layer boundaries: Generic game layer does not include Doom or adapter headers",
          "[layer][game]") {
  require_no_includes_matching(
      fs::path(DMCP_SOURCE_DIR) / "src" / "game",
      {"dmcp/doom/", "dmcp/adapter/", "dmcp/adapters/", "doom/"},
      "Generic game files must stay game-agnostic and not include Doom or adapter headers. "
      "Violations:");

  require_no_includes_matching(
      fs::path(DMCP_SOURCE_DIR) / "include" / "mcp" / "game",
      {"dmcp/doom/", "dmcp/adapter/", "dmcp/adapters/", "doom/"},
      "Generic game public headers must stay game-agnostic and not include Doom or adapter "
      "headers. Violations:");
}

TEST_CASE("Layer boundaries: Generic MCP does not include doom headers", "[layer][mcp]") {
  fs::path              mcp_src_dir = fs::path(DMCP_SOURCE_DIR) / "src" / "mcp";
  std::vector<fs::path> violations;

  if (!fs::exists(mcp_src_dir)) {
    FAIL_CHECK("Could not locate src/mcp directory");
    return;
  }

  for (const auto& entry : fs::recursive_directory_iterator(mcp_src_dir)) {
    if (!entry.is_regular_file()) {
      continue;
    }
    if (!is_cpp_file(entry.path()) && !is_header_file(entry.path())) {
      continue;
    }

    std::string content  = read_file_content(entry.path());
    auto        includes = extract_includes(content);

    if (contains_include(includes, "dmcp/doom/") || contains_include(includes, "doom/")) {
      violations.push_back(entry.path());
    }
  }

  if (!violations.empty()) {
    std::string msg =
        "Generic MCP files must stay game-agnostic and not include doom headers. Violations:\n";
    for (const auto& v : violations) {
      msg += "  " + v.string() + "\n";
    }
    FAIL_CHECK(msg);
  }
}

TEST_CASE("Layer boundaries: Public headers keep ownership boundaries", "[layer][headers]") {
  fs::path include_dir = fs::path(DMCP_SOURCE_DIR) / "include";

  if (!fs::exists(include_dir)) {
    FAIL_CHECK("Could not locate include directory");
    return;
  }

  std::vector<fs::path> generic_violations;
  std::vector<fs::path> doom_violations;

  for (const auto& entry : fs::recursive_directory_iterator(include_dir)) {
    if (!entry.is_regular_file() || !is_header_file(entry.path())) {
      continue;
    }

    std::string content  = read_file_content(entry.path());
    auto        includes = extract_includes(content);

    if (path_contains(entry.path(), "generic") &&
        (contains_include(includes, "dmcp/") || contains_include(includes, "doom/"))) {
      generic_violations.push_back(entry.path());
    }

    if (path_contains(entry.path(), "doom") &&
        (contains_include(includes, "dmcp/adapters/") ||
         contains_include(includes, "adapters/crispy-doom/") ||
         contains_include(includes, "adapters/zdoom/"))) {
      doom_violations.push_back(entry.path());
    }
  }

  if (!generic_violations.empty()) {
    std::string msg =
        "Generic public headers must not include Doom or adapter headers. "
        "Violations:\n";
    for (const auto& v : generic_violations) {
      msg += "  " + v.string() + "\n";
    }
    FAIL_CHECK(msg);
  }

  if (!doom_violations.empty()) {
    std::string msg = "Doom public headers must not include adapter headers. Violations:\n";
    for (const auto& v : doom_violations) {
      msg += "  " + v.string() + "\n";
    }
    FAIL_CHECK(msg);
  }
}

TEST_CASE("Layer boundaries: json_types.hpp is the only doom header including mcp/json/json.hpp",
          "[layer][json]") {
  fs::path doom_src_dir = fs::path(DMCP_SOURCE_DIR) / "src" / "doom";

  if (!fs::exists(doom_src_dir)) {
    FAIL_CHECK("Could not locate src/doom directory");
    return;
  }

  for (const auto& entry : fs::recursive_directory_iterator(doom_src_dir)) {
    if (!entry.is_regular_file()) {
      continue;
    }
    if (!is_header_file(entry.path())) {
      continue;
    }

    std::string filename = entry.path().filename().string();
    if (filename == "json_types.hpp") {
      std::string content  = read_file_content(entry.path());
      auto        includes = extract_includes(content);
      REQUIRE(contains_include(includes, "mcp/json/json.hpp"));
    }
  }
}
