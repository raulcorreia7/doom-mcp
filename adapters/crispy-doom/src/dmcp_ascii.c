// DMCP ASCII Renderer Implementation
// Converts pixel buffers to ASCII art for DMCP

#include "dmcp_ascii.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mcp/core/memory.h"

static const char* CHARSET_STANDARD =
    "$@B%8&WM#*oahkbdpqwmZO0QLCJUYXzcvunxrjft/\\|()1{}[]?-_+~<>i!lI;:,\"^`'. ";
static const char* CHARSET_BLOCK = " ░▒▓█";
static const char* CHARSET_BRAILLE =
    " ⠀⠁⠂⠃⠄⠅⠆⠇⠈⠉⠊⠋⠌⠍⠎⠏⠐⠑⠒⠓⠔⠕⠖⠗⠘⠙⠚⠛⠜⠝⠞⠟⠠⠡⠢⠣⠤⠥⠦⠧⠨⠩⠪⠫⠬⠭⠮⠯⠰⠱⠲⠳⠴⠵⠶⠷⠸⠹⠺⠻⠼⠽⠾⠿";

static char*  g_output_buffer = NULL;
static size_t g_output_size   = 0;

static char* g_last_ascii      = NULL;
static bool  g_preview_enabled = false;

void dmcp_ascii_config_init(dmcp_ascii_config_t* cfg) {
  if (!cfg) return;
  memset(cfg, 0, sizeof(dmcp_ascii_config_t));
  cfg->rgb_buffers[0]  = NULL;
  cfg->rgb_buffers[1]  = NULL;
  cfg->rgb_buffer_size = 0;
  cfg->rgb_buffer_idx  = 0;
  dmcp_ascii_config_default(cfg);
}

void dmcp_ascii_config_free(dmcp_ascii_config_t* cfg) {
  if (!cfg) return;
  free(cfg->rgb_buffers[0]);
  free(cfg->rgb_buffers[1]);
  free(g_output_buffer);
  cfg->rgb_buffers[0] = NULL;
  cfg->rgb_buffers[1] = NULL;
  g_output_buffer     = NULL;
  g_output_size       = 0;
}

void dmcp_ascii_config_default(dmcp_ascii_config_t* cfg) {
  if (!cfg) return;
  cfg->charset       = DMCP_ASCII_CHARSET_STANDARD;
  cfg->color         = DMCP_ASCII_COLOR_24BIT;
  cfg->width         = 0;
  cfg->height        = 0;
  cfg->scale         = 1;
  cfg->use_gradients = 1;
  cfg->use_bold      = 1;
  cfg->gamma         = 0;
}

const char* dmcp_ascii_get_output(void) { return g_output_buffer; }

static float calc_brightness(uint8_t r, uint8_t g, uint8_t b, int gamma) {
  float br = r * 0.299f + g * 0.587f + b * 0.114f;
  if (gamma != 0) {
    br = 255.0f * powf(br / 255.0f, 1.0f / (1.0f + (float)gamma * 0.1f));
  }
  return br;
}

static const char* get_charset(dmcp_ascii_charset_t charset, int* count) {
  switch (charset) {
    case DMCP_ASCII_CHARSET_BLOCK:
      *count = 5;
      return CHARSET_BLOCK;
    case DMCP_ASCII_CHARSET_BRAILLE:
      *count = 64;
      return CHARSET_BRAILLE;
    default:
      *count = 69;
      return CHARSET_STANDARD;
  }
}

static char brightness_to_char(float brightness, const char* charset, int count, int gradients) {
  int idx;

  if (!gradients || count <= 1) return charset[count - 1];
  idx = (int)((brightness / 255.0f) * (float)(count - 1));
  if (idx < 0) idx = 0;
  if (idx >= count) idx = count - 1;
  return charset[idx];
}

static int rgb_to_ansi256(uint8_t r, uint8_t g, uint8_t b) {
  if (abs(r - g) < 3 && abs(g - b) < 3) {
    int gray = (r * 299 + g * 587 + b * 114) / 1000;
    if (gray < 24) return 16;
    if (gray > 231) return 231;
    return 232 + (gray - 24) / 8;
  }
  return 16 + (r / 51 * 36) + (g / 51 * 6) + (b / 51);
}

const char* dmcp_ascii_render(const uint8_t* pixels, int width, int height,
                              dmcp_ascii_config_t* cfg) {
  int            cfg_width;
  int            cfg_height;
  float          scale_x;
  float          scale_y;
  int            charset_count;
  const char*    charset;
  size_t         rgb_needed;
  uint8_t*       rgb;
  int            max_escape_len;
  size_t         out_needed;
  char*          out;
  int            out_len;
  int            y;
  int            x;
  int            py;
  int            px;
  size_t         width_sz;
  size_t         height_sz;
  size_t         pixel_count;
  const uint8_t* p;
  uint8_t        r, g, b;
  float          brightness;
  char           ch;

  if (!pixels || width <= 0 || height <= 0 || !cfg) {
    return NULL;
  }

  width_sz  = (size_t)width;
  height_sz = (size_t)height;
  if (height_sz != 0 && width_sz > SIZE_MAX / height_sz) {
    return NULL;
  }
  pixel_count = width_sz * height_sz;
  if (pixel_count > SIZE_MAX / 3) {
    return NULL;
  }
  rgb_needed = pixel_count * 3;

  cfg_width  = cfg->width > 0 ? cfg->width : width;
  cfg_height = cfg->height > 0 ? cfg->height : height;
  cfg_width /= cfg->scale;
  cfg_height /= cfg->scale;

  if (cfg_width < 40) cfg_width = 40;
  if (cfg_height < 25) cfg_height = 25;

  scale_x = (float)width / (float)cfg_width;
  scale_y = (float)height / (float)cfg_height;

  charset_count = 0;
  charset       = get_charset(cfg->charset, &charset_count);

  if (cfg->rgb_buffer_size < rgb_needed) {
    free(cfg->rgb_buffers[0]);
    free(cfg->rgb_buffers[1]);
    cfg->rgb_buffers[0] = (uint8_t*)malloc(rgb_needed);
    cfg->rgb_buffers[1] = (uint8_t*)malloc(rgb_needed);
    if (!cfg->rgb_buffers[0] || !cfg->rgb_buffers[1]) {
      free(cfg->rgb_buffers[0]);
      free(cfg->rgb_buffers[1]);
      cfg->rgb_buffers[0] = cfg->rgb_buffers[1] = NULL;
      cfg->rgb_buffer_size                      = 0;
      return NULL;
    }
    cfg->rgb_buffer_size = rgb_needed;
  }

  rgb = cfg->rgb_buffers[cfg->rgb_buffer_idx];
  if (!mcp_memcpy_safe(rgb, cfg->rgb_buffer_size, pixels, rgb_needed)) {
    return NULL;
  }
  cfg->rgb_buffer_idx = 1 - cfg->rgb_buffer_idx;

  max_escape_len = cfg->color == DMCP_ASCII_COLOR_24BIT ? 32 : 20;
  if ((size_t)cfg_height != 0 && (size_t)cfg_width > SIZE_MAX / (size_t)cfg_height) {
    return NULL;
  }
  out_needed = (size_t)cfg_width * (size_t)cfg_height;
  if (out_needed > (SIZE_MAX - 256) / (size_t)(max_escape_len + 4)) {
    return NULL;
  }
  out_needed = out_needed * (size_t)(max_escape_len + 4) + 256;
  if (g_output_size < out_needed) {
    free(g_output_buffer);
    g_output_buffer = (char*)malloc(out_needed);
    if (!g_output_buffer) {
      g_output_size = 0;
      return NULL;
    }
    g_output_size = out_needed;
  }

  out     = g_output_buffer;
  out_len = 0;

  for (y = 0; y < cfg_height; y++) {
    py = (int)((float)y * scale_y);
    if (py >= height) py = height - 1;

    for (x = 0; x < cfg_width; x++) {
      px = (int)((float)x * scale_x);
      if (px >= width) px = width - 1;

      p = rgb + (py * width + px) * 3;
      r = p[0];
      g = p[1];
      b = p[2];

      brightness = calc_brightness(r, g, b, cfg->gamma);
      ch         = brightness_to_char(brightness, charset, charset_count, cfg->use_gradients);

      if (cfg->color == DMCP_ASCII_COLOR_NONE) {
        out_len += snprintf(out + out_len, 2, "%c", ch);
      } else if (cfg->color == DMCP_ASCII_COLOR_8BIT) {
        int color = rgb_to_ansi256(r, g, b);
        if (cfg->use_bold) {
          out_len += snprintf(out + out_len, 24, "\033[1;38;5;%dm%c\033[0m", color, ch);
        } else {
          out_len += snprintf(out + out_len, 20, "\033[38;5;%dm%c\033[0m", color, ch);
        }
      } else {
        if (cfg->use_bold) {
          out_len += snprintf(out + out_len, 32, "\033[1;38;2;%d;%d;%dm%c\033[0m", r, g, b, ch);
        } else {
          out_len += snprintf(out + out_len, 28, "\033[38;2;%d;%d;%dm%c\033[0m", r, g, b, ch);
        }
      }
    }
    out[out_len++] = '\n';
  }

  out[out_len] = '\0';
  return g_output_buffer;
}

// Crispy Doom integration helpers

char* dmcp_crispy_render_ascii(const uint8_t* pixels, int width, int height, int target_width) {
  dmcp_ascii_config_t cfg;
  const char*         result;

  dmcp_ascii_config_init(&cfg);
  cfg.width    = target_width;
  cfg.height   = (int)((float)height * (float)target_width / (float)width * 0.5f);
  cfg.color    = DMCP_ASCII_COLOR_NONE;
  cfg.use_bold = 0;

  result = dmcp_ascii_render(pixels, width, height, &cfg);
  dmcp_ascii_config_free(&cfg);

  return result ? strdup(result) : NULL;
}

void dmcp_crispy_ascii_set_last(char* ascii) {
  free(g_last_ascii);
  g_last_ascii = ascii;
}

const char* dmcp_crispy_ascii_get_last(void) { return g_last_ascii; }

bool dmcp_crispy_ascii_preview_enabled(void) { return g_preview_enabled; }

void dmcp_crispy_ascii_preview_set(bool enabled) { g_preview_enabled = enabled; }
