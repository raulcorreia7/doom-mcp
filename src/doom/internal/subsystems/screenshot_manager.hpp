#pragma once
#include <atomic>
#include <mutex>
#include <vector>

#include "dmcp/doom/types.h"

namespace dmcp {

// Forward declaration
struct context;

class screenshot_manager {
 public:
  explicit screenshot_manager(context* ctx);
  ~screenshot_manager();

  bool initialize(bool enabled);
  void shutdown();

  bool is_enabled() const { return enabled_.load(); }

  bool is_requested() const;
  bool submit(const dmcp_screenshot_frame_t* frame);

  bool                        get_dimensions(uint32_t* width, uint32_t* height) const;
  const char*                 get_ascii(uint32_t target_width);
  const std::vector<uint8_t>& get_pixels() const;

  uint64_t get_dropped_count() const { return dropped_count_.load(); }

 private:
  context* ctx_;

  std::atomic<bool>     enabled_{false};
  std::atomic<uint32_t> pending_requests_{0};

  uint32_t             width_  = 0;
  uint32_t             height_ = 0;
  std::vector<uint8_t> pixels_;
  mutable std::mutex   pixels_mutex_;

  std::atomic<uint64_t> dropped_count_{0};
};

}  // namespace dmcp
