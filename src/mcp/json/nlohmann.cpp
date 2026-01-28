#include "json.hpp"

#include <nlohmann/json.hpp>

namespace mcp {
namespace json {

using njson = nlohmann::json;

// ============================================================================
// Document Implementation
// ============================================================================
class Document::Impl {
public:
  std::unique_ptr<njson> value;
};

Document::Document() : impl(std::make_unique<Impl>()) {}
Document::~Document() = default;

bool Document::parse(std::string_view json) {
  try {
    impl->value = std::make_unique<njson>(njson::parse(json));
    return true;
  } catch (...) {
    return false;
  }
}

void Document::create_object() {
  impl->value = std::make_unique<njson>(njson::object());
}

void Document::create_array() {
  impl->value = std::make_unique<njson>(njson::array());
}

Value Document::root() const {
  if (!impl->value) return Value();
  return Value(impl->value.get(), const_cast<Document*>(this));
}

std::string Document::dump(bool pretty) const {
  if (!impl->value) return {};
  if (pretty) {
    return impl->value->dump(2);
  }
  return impl->value->dump();
}

// ============================================================================
// Value Implementation
// ============================================================================
Value::Value(void* ptr, Document* doc) : ptr_(ptr), doc_(doc) {}

bool Value::is_null() const {
  if (!ptr_) return true;
  return static_cast<njson*>(ptr_)->is_null();
}

bool Value::is_bool() const {
  if (!ptr_) return false;
  return static_cast<njson*>(ptr_)->is_boolean();
}

bool Value::is_number() const {
  if (!ptr_) return false;
  return static_cast<njson*>(ptr_)->is_number();
}

bool Value::is_string() const {
  if (!ptr_) return false;
  return static_cast<njson*>(ptr_)->is_string();
}

bool Value::is_array() const {
  if (!ptr_) return false;
  return static_cast<njson*>(ptr_)->is_array();
}

bool Value::is_object() const {
  if (!ptr_) return false;
  return static_cast<njson*>(ptr_)->is_object();
}

bool Value::get_bool(bool default_val) const {
  if (!ptr_) return default_val;
  try {
    return static_cast<njson*>(ptr_)->get<bool>();
  } catch (...) {
    return default_val;
  }
}

int64_t Value::get_int(int64_t default_val) const {
  if (!ptr_) return default_val;
  try {
    return static_cast<njson*>(ptr_)->get<int64_t>();
  } catch (...) {
    return default_val;
  }
}

double Value::get_double(double default_val) const {
  if (!ptr_) return default_val;
  try {
    return static_cast<njson*>(ptr_)->get<double>();
  } catch (...) {
    return default_val;
  }
}

std::string_view Value::get_string(std::string_view default_val) const {
  if (!ptr_) return default_val;
  try {
    auto& str = static_cast<njson*>(ptr_)->get_ref<std::string&>();
    return str;
  } catch (...) {
    return default_val;
  }
}

Value Value::operator[](std::string_view key) const {
  if (!ptr_ || !doc_) return Value();
  auto it = static_cast<njson*>(ptr_)->find(std::string(key));
  if (it == static_cast<njson*>(ptr_)->end()) return Value();
  return Value(&*it, doc_);
}

bool Value::has_member(std::string_view key) const {
  if (!ptr_) return false;
  return static_cast<njson*>(ptr_)->contains(std::string(key));
}

void Value::set_member(std::string_view key, bool val) {
  if (!ptr_) return;
  (*static_cast<njson*>(ptr_))[std::string(key)] = val;
}

void Value::set_member(std::string_view key, int64_t val) {
  if (!ptr_) return;
  (*static_cast<njson*>(ptr_))[std::string(key)] = val;
}

void Value::set_member(std::string_view key, double val) {
  if (!ptr_) return;
  (*static_cast<njson*>(ptr_))[std::string(key)] = val;
}

void Value::set_member(std::string_view key, std::string_view val) {
  if (!ptr_) return;
  (*static_cast<njson*>(ptr_))[std::string(key)] = std::string(val);
}

void Value::set_member(std::string_view key, const Value& val) {
  if (!ptr_ || !val.ptr_) return;
  (*static_cast<njson*>(ptr_))[std::string(key)] = *static_cast<njson*>(val.ptr_);
}

size_t Value::size() const {
  if (!ptr_) return 0;
  return static_cast<njson*>(ptr_)->size();
}

Value Value::operator[](size_t index) const {
  if (!ptr_ || !doc_) return Value();
  if (index >= size()) return Value();
  return Value(&(*static_cast<njson*>(ptr_))[index], doc_);
}

void Value::push_back(bool val) {
  if (!ptr_) return;
  static_cast<njson*>(ptr_)->push_back(val);
}

void Value::push_back(int64_t val) {
  if (!ptr_) return;
  static_cast<njson*>(ptr_)->push_back(val);
}

void Value::push_back(double val) {
  if (!ptr_) return;
  static_cast<njson*>(ptr_)->push_back(val);
}

void Value::push_back(std::string_view val) {
  if (!ptr_) return;
  static_cast<njson*>(ptr_)->push_back(std::string(val));
}

void Value::push_back(const Value& val) {
  if (!ptr_ || !val.ptr_) return;
  static_cast<njson*>(ptr_)->push_back(*static_cast<njson*>(val.ptr_));
}

std::vector<Value::KeyValue> Value::members() const {
  std::vector<KeyValue> result;
  if (!ptr_ || !is_object()) return result;
  
  for (auto& [key, val] : static_cast<njson*>(ptr_)->items()) {
    result.emplace_back(key, Value(&val, doc_));
  }
  return result;
}

std::vector<Value> Value::elements() const {
  std::vector<Value> result;
  if (!ptr_ || !is_array()) return result;
  
  size_t n = size();
  result.reserve(n);
  for (size_t i = 0; i < n; i++) {
    result.push_back(operator[](i));
  }
  return result;
}

// ============================================================================
// Builder Implementation
// ============================================================================
class Builder::Impl {
public:
  std::vector<njson> stack;
  njson root;
};

Builder::Builder() : impl(std::make_unique<Impl>()) {}
Builder::~Builder() = default;

Builder::Builder(Builder&&) = default;
Builder& Builder::operator=(Builder&&) = default;

void Builder::start_object() {
  if (!impl->stack.empty()) {
    impl->stack.push_back(njson::object());
  } else {
    impl->root = njson::object();
    impl->stack.push_back(impl->root);
  }
}

void Builder::start_array() {
  if (!impl->stack.empty()) {
    impl->stack.push_back(njson::array());
  } else {
    impl->root = njson::array();
    impl->stack.push_back(impl->root);
  }
}

void Builder::add(std::string_view key, bool val) {
  if (impl->stack.empty()) return;
  impl->stack.back()[std::string(key)] = val;
}

void Builder::add(std::string_view key, int64_t val) {
  if (impl->stack.empty()) return;
  impl->stack.back()[std::string(key)] = val;
}

void Builder::add(std::string_view key, double val) {
  if (impl->stack.empty()) return;
  impl->stack.back()[std::string(key)] = val;
}

void Builder::add(std::string_view key, std::string_view val) {
  if (impl->stack.empty()) return;
  impl->stack.back()[std::string(key)] = std::string(val);
}

void Builder::add(std::string_view key, const char* val) {
  add(key, std::string_view(val));
}

void Builder::add(std::string_view key, const Builder& nested) {
  if (impl->stack.empty() || nested.impl->stack.empty()) return;
  impl->stack.back()[std::string(key)] = nested.impl->stack.back();
}

void Builder::push(bool val) {
  if (impl->stack.empty()) return;
  impl->stack.back().push_back(val);
}

void Builder::push(int64_t val) {
  if (impl->stack.empty()) return;
  impl->stack.back().push_back(val);
}

void Builder::push(double val) {
  if (impl->stack.empty()) return;
  impl->stack.back().push_back(val);
}

void Builder::push(std::string_view val) {
  if (impl->stack.empty()) return;
  impl->stack.back().push_back(std::string(val));
}

void Builder::push(const char* val) {
  push(std::string_view(val));
}

void Builder::push(const Builder& nested) {
  if (impl->stack.empty() || nested.impl->stack.empty()) return;
  impl->stack.back().push_back(nested.impl->stack.back());
}

std::string Builder::finish() {
  if (impl->stack.empty()) return {};
  return impl->stack.back().dump();
}

} // namespace json
} // namespace mcp
