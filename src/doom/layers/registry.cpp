#include "doom/layers/registry.hpp"

#include <new>
#include <utility>

namespace dmcp {

layer_registry::layer_registry() = default;

layer_registry::~layer_registry() { destroy_all_layers(); }

void layer_registry::destroy_layer(dmcp_layer_t* layer) {
  if (!layer) {
    return;
  }

  if (layer->vtable && layer->vtable->destroy) {
    layer->vtable->destroy(layer);
    return;
  }

  delete layer;
}

bool layer_registry::register_layer(dmcp_layer_t* layer) {
  if (!layer || !layer->vtable || !layer->vtable->name) {
    return false;
  }

  const char* name = layer->vtable->name();
  if (!name || name[0] == '\0') {
    return false;
  }

  std::lock_guard<std::mutex> lock(mutex_);
  if (name_to_index_.find(name) != name_to_index_.end()) {
    return false;
  }

  layer_entry entry;
  entry.layer       = layer;
  entry.enabled     = false;
  entry.name        = name;
  entry.description = layer->vtable->description ? layer->vtable->description() : nullptr;
  entry.tool_count  = layer->vtable->tool_count ? layer->vtable->tool_count() : 0;

  name_to_index_[name] = layers_.size();
  layers_.push_back(entry);

  return true;
}

bool layer_registry::enable(const char* name) {
  if (!name) return false;

  std::lock_guard<std::mutex> lock(mutex_);
  auto                        it = name_to_index_.find(name);
  if (it == name_to_index_.end()) {
    return false;
  }

  layers_[it->second].enabled = true;
  return true;
}

bool layer_registry::disable(const char* name) {
  if (!name) return false;

  std::lock_guard<std::mutex> lock(mutex_);
  auto                        it = name_to_index_.find(name);
  if (it == name_to_index_.end()) {
    return false;
  }

  layers_[it->second].enabled = false;
  return true;
}

bool layer_registry::is_enabled(const char* name) const {
  if (!name) {
    return false;
  }

  std::lock_guard<std::mutex> lock(mutex_);
  const auto                  it = name_to_index_.find(name);
  if (it == name_to_index_.end()) {
    return false;
  }

  return layers_[it->second].enabled;
}

void layer_registry::tick_all(dmcp_context_t* ctx) {
  if (!ctx) return;

  std::lock_guard<std::mutex> lock(mutex_);
  for (auto& entry : layers_) {
    if (entry.enabled && entry.layer && entry.layer->vtable && entry.layer->vtable->tick) {
      entry.layer->vtable->tick(entry.layer, ctx);
    }
  }
}

size_t layer_registry::layer_count() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return layers_.size();
}

const layer_entry* layer_registry::get_layer(size_t index) const {
  std::lock_guard<std::mutex> lock(mutex_);
  if (index >= layers_.size()) {
    return nullptr;
  }
  return &layers_[index];
}

const layer_entry* layer_registry::find_layer(const char* name) const {
  if (!name) return nullptr;

  std::lock_guard<std::mutex> lock(mutex_);
  auto                        it = name_to_index_.find(name);
  if (it == name_to_index_.end()) {
    return nullptr;
  }
  return &layers_[it->second];
}

void layer_registry::destroy_all_layers() {
  std::vector<layer_entry> layers_to_destroy;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    layers_to_destroy.swap(layers_);
    name_to_index_.clear();
  }

  for (auto& entry : layers_to_destroy) {
    destroy_layer(entry.layer);
  }
}

}  // namespace dmcp

extern "C" {

dmcp_layer_registry_t* dmcp_layer_registry_create(void) {
  auto registry = new (std::nothrow) dmcp::layer_registry();
  return reinterpret_cast<dmcp_layer_registry_t*>(registry);
}

void dmcp_layer_registry_destroy(dmcp_layer_registry_t* registry) {
  if (!registry) return;
  auto* r = reinterpret_cast<dmcp::layer_registry*>(registry);
  delete r;
}

bool dmcp_layer_registry_register(dmcp_layer_registry_t* registry, dmcp_layer_t* layer) {
  if (!registry || !layer) return false;
  auto* r = reinterpret_cast<dmcp::layer_registry*>(registry);
  return r->register_layer(layer);
}

bool dmcp_layer_registry_enable(dmcp_layer_registry_t* registry, const char* name) {
  if (!registry || !name) return false;
  auto* r = reinterpret_cast<dmcp::layer_registry*>(registry);
  return r->enable(name);
}

bool dmcp_layer_registry_disable(dmcp_layer_registry_t* registry, const char* name) {
  if (!registry || !name) return false;
  auto* r = reinterpret_cast<dmcp::layer_registry*>(registry);
  return r->disable(name);
}

bool dmcp_layer_registry_is_enabled(const dmcp_layer_registry_t* registry, const char* name) {
  if (!registry || !name) {
    return false;
  }

  const auto* r = reinterpret_cast<const dmcp::layer_registry*>(registry);
  return r->is_enabled(name);
}

size_t dmcp_layer_registry_count(const dmcp_layer_registry_t* registry) {
  if (!registry) {
    return 0;
  }

  const auto* r = reinterpret_cast<const dmcp::layer_registry*>(registry);
  return r->layer_count();
}

void dmcp_layer_registry_tick_all(dmcp_layer_registry_t* registry, dmcp_context_t* ctx) {
  if (!registry || !ctx) return;
  auto* r = reinterpret_cast<dmcp::layer_registry*>(registry);
  r->tick_all(ctx);
}
}
