#include <yyjson.h>

#include <cstring>

#include "json.hpp"

namespace mcp {
namespace json {

namespace {

yyjson_mut_val* make_copied_key(yyjson_mut_doc* doc, std::string_view key) {
  if (!doc) {
    return nullptr;
  }
  return yyjson_mut_strncpy(doc, key.data(), key.size());
}

bool add_object_member(yyjson_mut_doc* doc, yyjson_mut_val* obj, std::string_view key,
                       yyjson_mut_val* value) {
  if (!doc || !obj || !value) {
    return false;
  }

  yyjson_mut_val* copied_key = make_copied_key(doc, key);
  if (!copied_key) {
    return false;
  }

  return yyjson_mut_obj_add(obj, copied_key, value);
}

}  // namespace

// ============================================================================
// Document Implementation
// ============================================================================
class Document::Impl {
 public:
  yyjson_doc*     doc     = nullptr;
  yyjson_mut_doc* mut_doc = nullptr;
  bool            owned   = true;

  ~Impl() {
    if (owned) {
      if (doc) yyjson_doc_free(doc);
      if (mut_doc) yyjson_mut_doc_free(mut_doc);
    }
  }
};

Document::Document() : impl(std::make_unique<Impl>()) {}
Document::~Document() = default;

bool Document::parse(std::string_view json) {
  if (impl->doc) {
    yyjson_doc_free(impl->doc);
    impl->doc = nullptr;
  }
  if (impl->mut_doc) {
    yyjson_mut_doc_free(impl->mut_doc);
    impl->mut_doc = nullptr;
  }
  impl->doc = yyjson_read(json.data(), json.size(), 0);
  return impl->doc != nullptr;
}

void Document::create_object() {
  if (impl->doc) {
    yyjson_doc_free(impl->doc);
    impl->doc = nullptr;
  }
  if (impl->mut_doc) yyjson_mut_doc_free(impl->mut_doc);
  impl->mut_doc        = yyjson_mut_doc_new(nullptr);
  yyjson_mut_val* root = yyjson_mut_obj(impl->mut_doc);
  yyjson_mut_doc_set_root(impl->mut_doc, root);
}

void Document::create_array() {
  if (impl->doc) {
    yyjson_doc_free(impl->doc);
    impl->doc = nullptr;
  }
  if (impl->mut_doc) yyjson_mut_doc_free(impl->mut_doc);
  impl->mut_doc        = yyjson_mut_doc_new(nullptr);
  yyjson_mut_val* root = yyjson_mut_arr(impl->mut_doc);
  yyjson_mut_doc_set_root(impl->mut_doc, root);
}

Value Document::root() const {
  if (impl->doc) {
    return Value(yyjson_doc_get_root(impl->doc), const_cast<Document*>(this));
  } else if (impl->mut_doc) {
    return Value(yyjson_mut_doc_get_root(impl->mut_doc), const_cast<Document*>(this));
  }
  return Value();
}

std::string Document::dump(bool pretty) const {
  if (impl->doc) {
    yyjson_write_flag flag = pretty ? YYJSON_WRITE_PRETTY : 0;
    char*             str  = yyjson_write(impl->doc, flag, nullptr);
    if (str) {
      std::string result(str);
      free(str);
      return result;
    }
  } else if (impl->mut_doc) {
    yyjson_write_flag flag = pretty ? YYJSON_WRITE_PRETTY : 0;
    char*             str  = yyjson_mut_write(impl->mut_doc, flag, nullptr);
    if (str) {
      std::string result(str);
      free(str);
      return result;
    }
  }
  return {};
}

// ============================================================================
// Value Implementation
// ============================================================================
Value::Value(void* ptr, Document* doc) : ptr_(ptr), doc_(doc) {}

bool Value::is_null() const {
  if (!ptr_) return true;
  if (doc_ && doc_->impl->doc) {
    return yyjson_is_null(static_cast<yyjson_val*>(ptr_));
  }
  return yyjson_mut_is_null(static_cast<yyjson_mut_val*>(ptr_));
}

bool Value::is_bool() const {
  if (!ptr_) return false;
  if (doc_ && doc_->impl->doc) {
    return yyjson_is_bool(static_cast<yyjson_val*>(ptr_));
  }
  return yyjson_mut_is_bool(static_cast<yyjson_mut_val*>(ptr_));
}

bool Value::is_number() const {
  if (!ptr_) return false;
  if (doc_ && doc_->impl->doc) {
    return yyjson_is_num(static_cast<yyjson_val*>(ptr_));
  }
  return yyjson_mut_is_num(static_cast<yyjson_mut_val*>(ptr_));
}

bool Value::is_string() const {
  if (!ptr_) return false;
  if (doc_ && doc_->impl->doc) {
    return yyjson_is_str(static_cast<yyjson_val*>(ptr_));
  }
  return yyjson_mut_is_str(static_cast<yyjson_mut_val*>(ptr_));
}

bool Value::is_array() const {
  if (!ptr_) return false;
  if (doc_ && doc_->impl->doc) {
    return yyjson_is_arr(static_cast<yyjson_val*>(ptr_));
  }
  return yyjson_mut_is_arr(static_cast<yyjson_mut_val*>(ptr_));
}

bool Value::is_object() const {
  if (!ptr_) return false;
  if (doc_ && doc_->impl->doc) {
    return yyjson_is_obj(static_cast<yyjson_val*>(ptr_));
  }
  return yyjson_mut_is_obj(static_cast<yyjson_mut_val*>(ptr_));
}

bool Value::get_bool(bool default_val) const {
  if (!ptr_) return default_val;
  if (doc_ && doc_->impl->doc) {
    return yyjson_get_bool(static_cast<yyjson_val*>(ptr_));
  }
  return yyjson_mut_get_bool(static_cast<yyjson_mut_val*>(ptr_));
}

int64_t Value::get_int(int64_t default_val) const {
  if (!ptr_) return default_val;
  if (doc_ && doc_->impl->doc) {
    return yyjson_get_sint(static_cast<yyjson_val*>(ptr_));
  }
  return yyjson_mut_get_sint(static_cast<yyjson_mut_val*>(ptr_));
}

double Value::get_double(double default_val) const {
  if (!ptr_) return default_val;
  if (doc_ && doc_->impl->doc) {
    return yyjson_get_real(static_cast<yyjson_val*>(ptr_));
  }
  return yyjson_mut_get_real(static_cast<yyjson_mut_val*>(ptr_));
}

std::string_view Value::get_string(std::string_view default_val) const {
  if (!ptr_) return default_val;
  if (doc_ && doc_->impl->doc) {
    const char* str = yyjson_get_str(static_cast<yyjson_val*>(ptr_));
    if (str) return str;
  } else {
    const char* str = yyjson_mut_get_str(static_cast<yyjson_mut_val*>(ptr_));
    if (str) return str;
  }
  return default_val;
}

Value Value::operator[](std::string_view key) const {
  if (!ptr_ || !doc_) return Value();
  if (doc_->impl->doc) {
    yyjson_val* val = yyjson_obj_getn(static_cast<yyjson_val*>(ptr_), key.data(), key.size());
    return Value(val, doc_);
  } else {
    yyjson_mut_val* val =
        yyjson_mut_obj_getn(static_cast<yyjson_mut_val*>(ptr_), key.data(), key.size());
    return Value(val, doc_);
  }
}

bool Value::has_member(std::string_view key) const {
  if (!ptr_ || !doc_) return false;
  if (doc_->impl->doc) {
    return yyjson_obj_getn(static_cast<yyjson_val*>(ptr_), key.data(), key.size()) != nullptr;
  } else {
    return yyjson_mut_obj_getn(static_cast<yyjson_mut_val*>(ptr_), key.data(), key.size()) !=
           nullptr;
  }
}

void Value::set_member(std::string_view key, bool val) {
  if (!ptr_ || !doc_ || !doc_->impl->mut_doc) return;
  yyjson_mut_val* obj   = static_cast<yyjson_mut_val*>(ptr_);
  yyjson_mut_val* value = yyjson_mut_bool(doc_->impl->mut_doc, val);
  add_object_member(doc_->impl->mut_doc, obj, key, value);
}

void Value::set_member(std::string_view key, int64_t val) {
  if (!ptr_ || !doc_ || !doc_->impl->mut_doc) return;
  yyjson_mut_val* obj   = static_cast<yyjson_mut_val*>(ptr_);
  yyjson_mut_val* value = yyjson_mut_sint(doc_->impl->mut_doc, val);
  add_object_member(doc_->impl->mut_doc, obj, key, value);
}

void Value::set_member(std::string_view key, double val) {
  if (!ptr_ || !doc_ || !doc_->impl->mut_doc) return;
  yyjson_mut_val* obj   = static_cast<yyjson_mut_val*>(ptr_);
  yyjson_mut_val* value = yyjson_mut_real(doc_->impl->mut_doc, val);
  add_object_member(doc_->impl->mut_doc, obj, key, value);
}

void Value::set_member(std::string_view key, const char* val) {
  set_member(key, std::string_view(val ? val : ""));
}

void Value::set_member(std::string_view key, std::string_view val) {
  if (!ptr_ || !doc_ || !doc_->impl->mut_doc) return;
  yyjson_mut_val* obj   = static_cast<yyjson_mut_val*>(ptr_);
  yyjson_mut_val* value = yyjson_mut_strncpy(doc_->impl->mut_doc, val.data(), val.size());
  add_object_member(doc_->impl->mut_doc, obj, key, value);
}

void Value::set_member(std::string_view key, const Value& val) {
  if (!ptr_ || !doc_ || !doc_->impl->mut_doc || !val.ptr_) return;
  yyjson_mut_val* obj  = static_cast<yyjson_mut_val*>(ptr_);
  yyjson_mut_val* copy = nullptr;
  if (val.doc_ && val.doc_->impl->doc) {
    copy = yyjson_val_mut_copy(doc_->impl->mut_doc, static_cast<yyjson_val*>(val.ptr_));
  } else if (val.doc_ && val.doc_->impl->mut_doc) {
    copy = yyjson_mut_val_mut_copy(doc_->impl->mut_doc, static_cast<yyjson_mut_val*>(val.ptr_));
  }
  add_object_member(doc_->impl->mut_doc, obj, key, copy);
}

size_t Value::size() const {
  if (!ptr_) return 0;
  if (doc_ && doc_->impl->doc) {
    return yyjson_arr_size(static_cast<yyjson_val*>(ptr_));
  }
  return yyjson_mut_arr_size(static_cast<yyjson_mut_val*>(ptr_));
}

Value Value::operator[](size_t index) const {
  if (!ptr_ || !doc_) return Value();
  if (doc_->impl->doc) {
    yyjson_val* val = yyjson_arr_get(static_cast<yyjson_val*>(ptr_), index);
    return Value(val, doc_);
  } else {
    yyjson_mut_val* val = yyjson_mut_arr_get(static_cast<yyjson_mut_val*>(ptr_), index);
    return Value(val, doc_);
  }
}

void Value::push_back(bool val) {
  if (!ptr_ || !doc_ || !doc_->impl->mut_doc) return;
  yyjson_mut_val* arr = static_cast<yyjson_mut_val*>(ptr_);
  yyjson_mut_arr_add_bool(doc_->impl->mut_doc, arr, val);
}

void Value::push_back(int64_t val) {
  if (!ptr_ || !doc_ || !doc_->impl->mut_doc) return;
  yyjson_mut_val* arr = static_cast<yyjson_mut_val*>(ptr_);
  yyjson_mut_arr_add_sint(doc_->impl->mut_doc, arr, val);
}

void Value::push_back(double val) {
  if (!ptr_ || !doc_ || !doc_->impl->mut_doc) return;
  yyjson_mut_val* arr = static_cast<yyjson_mut_val*>(ptr_);
  yyjson_mut_arr_add_real(doc_->impl->mut_doc, arr, val);
}

void Value::push_back(std::string_view val) {
  if (!ptr_ || !doc_ || !doc_->impl->mut_doc) return;
  yyjson_mut_val* arr = static_cast<yyjson_mut_val*>(ptr_);
  yyjson_mut_arr_add_strncpy(doc_->impl->mut_doc, arr, val.data(), val.size());
}

void Value::push_back(const Value& val) {
  if (!ptr_ || !doc_ || !doc_->impl->mut_doc || !val.ptr_) return;
  yyjson_mut_val* arr  = static_cast<yyjson_mut_val*>(ptr_);
  yyjson_mut_val* copy = nullptr;
  if (val.doc_ && val.doc_->impl->doc) {
    copy = yyjson_val_mut_copy(doc_->impl->mut_doc, static_cast<yyjson_val*>(val.ptr_));
  } else if (val.doc_ && val.doc_->impl->mut_doc) {
    copy = yyjson_mut_val_mut_copy(doc_->impl->mut_doc, static_cast<yyjson_mut_val*>(val.ptr_));
  }
  if (!copy) return;
  yyjson_mut_arr_append(arr, copy);
}

std::vector<Value::KeyValue> Value::members() const {
  std::vector<KeyValue> result;
  if (!ptr_ || !is_object()) return result;

  if (doc_ && doc_->impl->doc) {
    // Immutable document iteration
    yyjson_val* obj = static_cast<yyjson_val*>(ptr_);
    size_t      idx, max;
    yyjson_val *key, *val;
    yyjson_obj_foreach(obj, idx, max, key, val) {
      result.emplace_back(yyjson_get_str(key), Value(val, doc_));
    }
  } else if (doc_ && doc_->impl->mut_doc) {
    // Mutable document iteration
    yyjson_mut_val* obj = static_cast<yyjson_mut_val*>(ptr_);
    size_t          idx, max;
    yyjson_mut_val *key, *val;
    yyjson_mut_obj_foreach(obj, idx, max, key, val) {
      result.emplace_back(yyjson_mut_get_str(key), Value(val, doc_));
    }
  }
  return result;
}

std::string Value::dump() const {
  if (!ptr_) return "null";

  if (doc_ && doc_->impl->doc) {
    char* str = yyjson_val_write(static_cast<yyjson_val*>(ptr_), 0, nullptr);
    if (str) {
      std::string result(str);
      free(str);
      return result;
    }
  } else if (doc_ && doc_->impl->mut_doc) {
    char* str = yyjson_mut_val_write(static_cast<yyjson_mut_val*>(ptr_), 0, nullptr);
    if (str) {
      std::string result(str);
      free(str);
      return result;
    }
  }
  return "null";
}

// ============================================================================
// Builder Implementation
// ============================================================================
class Builder::Impl {
 public:
  yyjson_mut_doc*              doc = nullptr;
  std::vector<yyjson_mut_val*> stack;

  Impl() { doc = yyjson_mut_doc_new(nullptr); }

  ~Impl() {
    if (doc) yyjson_mut_doc_free(doc);
  }
};

Builder::Builder() : impl(std::make_unique<Impl>()) {}
Builder::~Builder() = default;

// Move constructor and assignment must be defined here where Impl is complete
Builder::Builder(Builder&&)            = default;
Builder& Builder::operator=(Builder&&) = default;

void Builder::start_object() {
  if (!impl->stack.empty()) {
    // Will be added to parent later
    yyjson_mut_val* obj = yyjson_mut_obj(impl->doc);
    impl->stack.push_back(obj);
  } else {
    yyjson_mut_val* obj = yyjson_mut_obj(impl->doc);
    yyjson_mut_doc_set_root(impl->doc, obj);
    impl->stack.push_back(obj);
  }
}

void Builder::start_array() {
  if (!impl->stack.empty()) {
    yyjson_mut_val* arr = yyjson_mut_arr(impl->doc);
    impl->stack.push_back(arr);
  } else {
    yyjson_mut_val* arr = yyjson_mut_arr(impl->doc);
    yyjson_mut_doc_set_root(impl->doc, arr);
    impl->stack.push_back(arr);
  }
}

void Builder::add(std::string_view key, bool val) {
  if (impl->stack.empty()) return;
  yyjson_mut_val* obj   = impl->stack.back();
  yyjson_mut_val* value = yyjson_mut_bool(impl->doc, val);
  add_object_member(impl->doc, obj, key, value);
}

void Builder::add(std::string_view key, int64_t val) {
  if (impl->stack.empty()) return;
  yyjson_mut_val* obj   = impl->stack.back();
  yyjson_mut_val* value = yyjson_mut_sint(impl->doc, val);
  add_object_member(impl->doc, obj, key, value);
}

void Builder::add(std::string_view key, double val) {
  if (impl->stack.empty()) return;
  yyjson_mut_val* obj   = impl->stack.back();
  yyjson_mut_val* value = yyjson_mut_real(impl->doc, val);
  add_object_member(impl->doc, obj, key, value);
}

void Builder::add(std::string_view key, std::string_view val) {
  if (impl->stack.empty()) return;
  yyjson_mut_val* obj   = impl->stack.back();
  yyjson_mut_val* value = yyjson_mut_strncpy(impl->doc, val.data(), val.size());
  add_object_member(impl->doc, obj, key, value);
}

void Builder::add(std::string_view key, const char* val) { add(key, std::string_view(val)); }

void Builder::add(std::string_view key, const Builder& nested) {
  if (impl->stack.empty() || nested.impl->stack.empty()) return;
  yyjson_mut_val* obj  = impl->stack.back();
  yyjson_mut_val* val  = nested.impl->stack.back();
  yyjson_mut_val* copy = yyjson_mut_val_mut_copy(impl->doc, val);
  add_object_member(impl->doc, obj, key, copy);
}

void Builder::push(bool val) {
  if (impl->stack.empty()) return;
  yyjson_mut_val* arr = impl->stack.back();
  yyjson_mut_arr_add_bool(impl->doc, arr, val);
}

void Builder::push(int64_t val) {
  if (impl->stack.empty()) return;
  yyjson_mut_val* arr = impl->stack.back();
  yyjson_mut_arr_add_sint(impl->doc, arr, val);
}

void Builder::push(double val) {
  if (impl->stack.empty()) return;
  yyjson_mut_val* arr = impl->stack.back();
  yyjson_mut_arr_add_real(impl->doc, arr, val);
}

void Builder::push(std::string_view val) {
  if (impl->stack.empty()) return;
  yyjson_mut_val* arr = impl->stack.back();
  yyjson_mut_arr_add_strncpy(impl->doc, arr, val.data(), val.size());
}

void Builder::push(const char* val) { push(std::string_view(val)); }

void Builder::push(const Builder& nested) {
  if (impl->stack.empty() || nested.impl->stack.empty()) return;
  yyjson_mut_val* arr  = impl->stack.back();
  yyjson_mut_val* val  = nested.impl->stack.back();
  yyjson_mut_val* copy = yyjson_mut_val_mut_copy(impl->doc, val);
  yyjson_mut_arr_append(arr, copy);
}

std::string Builder::finish() {
  if (impl->stack.empty()) return {};

  // Don't pop the root, just serialize it
  char* str = yyjson_mut_write(impl->doc, 0, nullptr);
  if (str) {
    std::string result(str);
    free(str);
    return result;
  }
  return {};
}

}  // namespace json
}  // namespace mcp
