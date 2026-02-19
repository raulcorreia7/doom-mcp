#pragma once

#include <cstddef>

namespace dmcp {

bool handle_tools_list(void* user_data, const char* method, const char* request_json,
                       char* response_buffer, size_t response_size);

bool handle_tools_call(void* user_data, const char* method, const char* request_json,
                       char* response_buffer, size_t response_size);

bool handle_method_get_game_state(void* user_data, const char* method, const char* request_json,
                                  char* response_buffer, size_t response_size);

bool handle_method_get_screenshot(void* user_data, const char* method, const char* request_json,
                                  char* response_buffer, size_t response_size);

bool handle_method_get_command_result(void* user_data, const char* method, const char* request_json,
                                      char* response_buffer, size_t response_size);

bool handle_method_execute_command(void* user_data, const char* method, const char* request_json,
                                   char* response_buffer, size_t response_size);

bool handle_route_game_state(void* user_data, const char* method, const char* path,
                             const char* body, char* response_buffer, size_t response_size,
                             int* http_status);

bool handle_route_game_screenshot(void* user_data, const char* method, const char* path,
                                  const char* body, char* response_buffer, size_t response_size,
                                  int* http_status);

}  // namespace dmcp
