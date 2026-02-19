#pragma once
#include <deque>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

#include "../command_queue.hpp"
#include "dmcp/doom/commands.h"

namespace dmcp {

// Forward declaration
struct context;

class command_manager {
 public:
  explicit command_manager(context* ctx);
  ~command_manager();

  bool push_command(const dmcp_command_t& cmd, uint64_t* assigned_sequence = nullptr);
  std::optional<dmcp_command_t> pop_command();
  bool                          has_commands() const;
  uint32_t                      command_count() const;
  void                          clear_commands();

  bool mark_queued(const dmcp_command_t& cmd);
  bool mark_complete(const dmcp_command_t& cmd, bool success, const char* message);
  bool get_result(uint64_t sequence, dmcp_command_result_t* out_result) const;

  bool register_custom_parser(const char* name, dmcp_command_type_t type,
                              dmcp_custom_command_parser_t parser);
  bool parse_custom_command(const char* name, const char* json, dmcp_command_t* out_cmd);

 private:
  context*                       ctx_;
  std::unique_ptr<command_queue> queue_;

  std::unordered_map<std::string, std::pair<dmcp_command_type_t, dmcp_custom_command_parser_t>>
                     custom_parsers_;
  mutable std::mutex custom_parsers_mutex_;

  std::unordered_map<uint64_t, dmcp_command_result_t> command_results_;
  std::deque<uint64_t>                                command_result_order_;
  mutable std::mutex                                  command_results_mutex_;
  size_t                                              max_command_results_ = 256;
};

}  // namespace dmcp
