#include <cstdio>
#include <cstring>
#include <string>

#include "internal/context.hpp"
#include "mcp/generic/constants.h"

namespace dmcp {

static void AppendString(char*& buf, size_t& remaining, const char* str) {
  size_t len = std::strlen(str);
  if (len >= remaining) len = remaining - 1;
  std::memcpy(buf, str, len);
  buf += len;
  remaining -= len;
  *buf = '\0';
}

static void AppendFloat(char*& buf, size_t& remaining, float val) {
  char temp[32];
  std::snprintf(temp, sizeof(temp), "%.2f", val);
  AppendString(buf, remaining, temp);
}

static void AppendInt(char*& buf, size_t& remaining, int val) {
  char temp[32];
  std::snprintf(temp, sizeof(temp), "%d", val);
  AppendString(buf, remaining, temp);
}

std::string snapshot_to_json(const dmcp_snapshot_t& snapshot) {
  char   buffer[MCP_MAX_JSON_SIZE];
  char*  p         = buffer;
  size_t remaining = sizeof(buffer);

  AppendString(p, remaining, "{");

  AppendString(p, remaining, "\"player\":{");
  AppendString(p, remaining, "\"hp\":");
  AppendFloat(p, remaining, snapshot.player.hp);
  AppendString(p, remaining, ",\"armor\":");
  AppendFloat(p, remaining, snapshot.player.armor);
  AppendString(p, remaining, ",\"ammo\":");
  AppendInt(p, remaining, snapshot.player.ammo);
  AppendString(p, remaining, ",\"position\":{");
  AppendString(p, remaining, "\"x\":");
  AppendFloat(p, remaining, snapshot.player.position.x);
  AppendString(p, remaining, ",\"y\":");
  AppendFloat(p, remaining, snapshot.player.position.y);
  AppendString(p, remaining, "}}");

  AppendString(p, remaining, ",\"inventory\":[");
  for (std::uint32_t i = 0; i < snapshot.inventory_count; i++) {
    if (i > 0) AppendString(p, remaining, ",");
    AppendString(p, remaining, "{\"name\":\"");
    for (const char* c = snapshot.inventory[i].name; *c; c++) {
      if (*c == '"' || *c == '\\') {
	if (remaining > 1) {
	  *p++ = '\\';
	  remaining--;
	}
      }
      if (remaining > 1) {
	*p++ = *c;
	remaining--;
      }
    }
    AppendString(p, remaining, "\",\"amount\":");
    AppendInt(p, remaining, snapshot.inventory[i].amount);
    AppendString(p, remaining, "}");
  }
  AppendString(p, remaining, "]");

  AppendString(p, remaining, ",\"level\":{");
  AppendString(p, remaining, "\"tic\":");
  AppendInt(p, remaining, snapshot.level.tic);
  AppendString(p, remaining, ",\"id\":\"");
  AppendString(p, remaining, snapshot.level.level_id);
  AppendString(p, remaining, "\",\"name\":\"");
  AppendString(p, remaining, snapshot.level.level_name);
  AppendString(p, remaining, "\",\"kill_count\":");
  AppendInt(p, remaining, snapshot.level.kill_count);
  AppendString(p, remaining, ",\"item_count\":");
  AppendInt(p, remaining, snapshot.level.item_count);
  AppendString(p, remaining, ",\"secret_count\":");
  AppendInt(p, remaining, snapshot.level.secret_count);
  AppendString(p, remaining, "}");

  AppendString(p, remaining, ",\"enemies\":[");
  for (std::uint32_t i = 0; i < snapshot.enemy_count; i++) {
    if (i > 0) AppendString(p, remaining, ",");
    AppendString(p, remaining, "{\"id\":");
    AppendInt(p, remaining, snapshot.enemies[i].id);
    AppendString(p, remaining, ",\"hp\":");
    AppendFloat(p, remaining, snapshot.enemies[i].hp);
    AppendString(p, remaining, ",\"max_hp\":");
    AppendFloat(p, remaining, snapshot.enemies[i].max_hp);
    AppendString(p, remaining, ",\"position\":{");
    AppendString(p, remaining, "\"x\":");
    AppendFloat(p, remaining, snapshot.enemies[i].position.x);
    AppendString(p, remaining, ",\"y\":");
    AppendFloat(p, remaining, snapshot.enemies[i].position.y);
    AppendString(p, remaining, "},\"type\":\"");
    for (const char* c = snapshot.enemies[i].type; *c; c++) {
      if (*c == '"' || *c == '\\') {
	if (remaining > 1) {
	  *p++ = '\\';
	  remaining--;
	}
      }
      if (remaining > 1) {
	*p++ = *c;
	remaining--;
      }
    }
    AppendString(p, remaining, "\"}");
  }
  AppendString(p, remaining, "]");

  AppendString(p, remaining, "}");

  return std::string{buffer, static_cast<size_t>(p - buffer)};
}

}  // namespace dmcp
