#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

// ============================================================================
// JSON Abstraction Layer
//
// Provides a common interface for JSON operations using yyjson.
// ============================================================================

namespace mcp {
namespace json {

class Value;

// ============================================================================
// Document - owns the parsed JSON
// ============================================================================
class Document {
 public:
  Document();
  ~Document();

  // Parse from string
  bool parse(std::string_view json);

  // Create empty object/array
  void create_object();
  void create_array();

  // Get root value
  Value root() const;

  // Serialize
  std::string dump(bool pretty = false) const;

  // Implementation detail
  class Impl;
  std::unique_ptr<Impl> impl;
};

// ============================================================================
// Value - reference to a JSON value
// ============================================================================
class Value {
 public:
  Value() = default;
  explicit Value(void* ptr, Document* doc);

  // Type checking
  bool is_null() const;
  bool is_bool() const;
  bool is_number() const;
  bool is_string() const;
  bool is_array() const;
  bool is_object() const;

  // Getters (with defaults)
  bool             get_bool(bool default_val = false) const;
  int64_t          get_int(int64_t default_val = 0) const;
  double           get_double(double default_val = 0.0) const;
  std::string_view get_string(std::string_view default_val = {}) const;

  // Object operations
  Value operator[](std::string_view key) const;
  bool  has_member(std::string_view key) const;
  void  set_member(std::string_view key, bool val);
  void  set_member(std::string_view key, int64_t val);
  void  set_member(std::string_view key, double val);
  void  set_member(std::string_view key, const char* val);
  void  set_member(std::string_view key, std::string_view val);
  void  set_member(std::string_view key, const Value& val);

  // Array operations
  size_t size() const;
  Value  operator[](size_t index) const;
  void   push_back(bool val);
  void   push_back(int64_t val);
  void   push_back(double val);
  void   push_back(std::string_view val);
  void   push_back(const Value& val);

  // Iteration for objects
  using KeyValue = std::pair<std::string_view, Value>;
  std::vector<KeyValue> members() const;

  // Check if value is valid
  explicit operator bool() const { return ptr_ != nullptr; }

  // Serialize this value to JSON string
  std::string dump() const;

 private:
  void*     ptr_ = nullptr;
  Document* doc_ = nullptr;
};

// ============================================================================
// Builder - for constructing JSON easily
// ============================================================================
class Builder {
 public:
  Builder();
  ~Builder();  // Defined in implementation

  // Move-only - defined in implementation due to unique_ptr with incomplete
  // type
  Builder(Builder&&);
  Builder& operator=(Builder&&);
  Builder(const Builder&)            = delete;
  Builder& operator=(const Builder&) = delete;

  // Start building
  void start_object();
  void start_array();

  // Add members to object
  void add(std::string_view key, bool val);
  void add(std::string_view key, int64_t val);
  void add(std::string_view key, double val);
  void add(std::string_view key, std::string_view val);
  void add(std::string_view key, const char* val);
  void add(std::string_view key, const Builder& nested);

  // Add to array
  void push(bool val);
  void push(int64_t val);
  void push(double val);
  void push(std::string_view val);
  void push(const char* val);
  void push(const Builder& nested);

  // Finish and get JSON string
  std::string finish();

  // Implementation
  class Impl;
  std::unique_ptr<Impl> impl;
};

// ============================================================================
// Helper Functions
// ============================================================================
inline std::string make_jsonrpc_error(std::string_view id, int code, std::string_view message) {
  Builder b;
  b.start_object();
  b.add("jsonrpc", "2.0");
  b.add("id", id);

  Builder err;
  err.start_object();
  err.add("code", static_cast<int64_t>(code));
  err.add("message", message);
  b.add("error", err);

  return b.finish();
}

}  // namespace json
}  // namespace mcp
