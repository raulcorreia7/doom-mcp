// DMCP ASCII Renderer - Converts pixel buffers to ASCII art
// Used for real-time preview in DMCP

#ifndef DMCP_ASCII_H
#define DMCP_ASCII_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  DMCP_ASCII_CHARSET_STANDARD = 0,
  DMCP_ASCII_CHARSET_BLOCK,
  DMCP_ASCII_CHARSET_BRAILLE
} dmcp_ascii_charset_t;

typedef enum {
  DMCP_ASCII_COLOR_NONE = 0,
  DMCP_ASCII_COLOR_8BIT,
  DMCP_ASCII_COLOR_24BIT
} dmcp_ascii_color_mode_t;

typedef struct {
  int                     enabled;
  dmcp_ascii_charset_t    charset;
  dmcp_ascii_color_mode_t color;
  int                     width;
  int                     height;
  int                     scale;
  int                     use_gradients;
  int                     use_bold;
  int                     gamma;

  uint8_t* rgb_buffers[2];
  int      rgb_buffer_size;
  int      rgb_buffer_idx;
} dmcp_ascii_config_t;

void dmcp_ascii_config_init(dmcp_ascii_config_t* cfg);
void dmcp_ascii_config_free(dmcp_ascii_config_t* cfg);
void dmcp_ascii_config_default(dmcp_ascii_config_t* cfg);

const char* dmcp_ascii_get_output(void);
const char* dmcp_ascii_render(const uint8_t* pixels, int width, int height,
                              dmcp_ascii_config_t* cfg);

// Chocolate Doom integration helpers
char* dmcp_chocolate_render_ascii(const uint8_t* pixels, int width, int height, int target_width);
void  dmcp_chocolate_ascii_set_last(char* ascii);
const char* dmcp_chocolate_ascii_get_last(void);
bool        dmcp_chocolate_ascii_preview_enabled(void);
void        dmcp_chocolate_ascii_preview_set(bool enabled);

#ifdef __cplusplus
}
#endif

#endif  // DMCP_ASCII_H
