#include "parsers.hpp"

#include <cstdint>
#include <limits>
#include <string_view>

#include "commands/parsers.hpp"

namespace dmcp {

bool parse_give_item_command(const json_value& params, dmcp_command_t* out) {
  out->type = DMCP_CMD_GIVE_ITEM;

  std::string_view item_class;
  if (!read_required_string(params, {"item_class", "item"}, &item_class) ||
      !copy_checked_string(out->data.give_item.item_class, sizeof(out->data.give_item.item_class),
                           item_class)) {
    return false;
  }

  std::int64_t amount = 1;
  if (!read_optional_int(params, {"amount", "quantity"}, 1, &amount) || amount < 1 ||
      amount > 1000) {
    return false;
  }
  out->data.give_item.amount = static_cast<std::int32_t>(amount);

  return true;
}

}  // namespace dmcp
