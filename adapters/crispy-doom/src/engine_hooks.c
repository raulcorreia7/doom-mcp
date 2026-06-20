#include "engine_hooks.h"
#include "i_system.h"
#include "i_video.h"
#include <stdlib.h>
#include <string.h>

static dmcp_crispy_t* g_dmcp_ctx = NULL;

dmcp_engine_config_t DMCP_ParseArgs(int argc, char** argv) {
  return dmcp_engine_config_from_argv(argc, argv);
}

void DMCP_Init(dmcp_engine_config_t cfg) {
  dmcp_crispy_config_t adapter_cfg   = dmcp_crispy_config_default();
  adapter_cfg.base.port              = (uint16_t)(cfg.port > 0 ? cfg.port : 6060);
  adapter_cfg.base.target_hz         = (uint32_t)(cfg.target_hz > 0 ? cfg.target_hz : 35);
  adapter_cfg.base.screenshot.enable = cfg.screenshot_enabled;
  adapter_cfg.base.permissions.allow_console_commands = cfg.allow_console_commands;
  adapter_cfg.base.permissions.allow_cheats           = cfg.allow_cheats;

  I_AtExit(DMCP_Shutdown, true);
  g_dmcp_ctx = dmcp_crispy_create(&adapter_cfg);
}

void DMCP_Shutdown(void) {
  if (g_dmcp_ctx) {
    dmcp_crispy_destroy(g_dmcp_ctx);
    g_dmcp_ctx = NULL;
  }
}

void DMCP_Tick(void) {
  if (g_dmcp_ctx) {
    dmcp_crispy_tick(g_dmcp_ctx);
    dmcp_crispy_commands_process(g_dmcp_ctx);
    dmcp_crispy_inputs_process(g_dmcp_ctx);
  }
}

static int DMCP_CopyFrameToRgba(const byte* src, int width, int height, int pitch,
                                unsigned char* dst) {
  int bytes_per_pixel;

  if (!src || !dst || width <= 0 || height <= 0 || pitch < width * 3) {
    return 0;
  }

  bytes_per_pixel = pitch >= width * 4 ? 4 : 3;

  for (int y = 0; y < height; ++y) {
    const byte* row = src + y * pitch;
    for (int x = 0; x < width; ++x) {
      const byte* pixel = row + x * bytes_per_pixel;
      size_t      out   = ((size_t)y * (size_t)width + (size_t)x) * 4u;
      dst[out + 0]      = pixel[0];
      dst[out + 1]      = pixel[1];
      dst[out + 2]      = pixel[2];
      dst[out + 3]      = 255u;
    }
  }

  return 1;
}

void DMCP_CaptureFrame(void) {
  dmcp_context_t* ctx;
  byte*           pixels = NULL;
  unsigned char*  rgba   = NULL;
  int             width  = 0;
  int             height = 0;
  int             pitch  = 0;

  if (!g_dmcp_ctx) {
    return;
  }

  ctx = dmcp_crispy_get_context(g_dmcp_ctx);
  if (!ctx || !dmcp_screenshot_is_requested(ctx)) {
    return;
  }

  I_RenderReadPixels(&pixels, &width, &height, &pitch);
  if (!pixels || width <= 0 || height <= 0 || pitch <= 0) {
    free(pixels);
    return;
  }

  rgba = (unsigned char*)malloc((size_t)width * (size_t)height * 4u);
  if (!rgba) {
    free(pixels);
    return;
  }

  if (DMCP_CopyFrameToRgba(pixels, width, height, pitch, rgba)) {
    dmcp_screenshot_frame_t frame;
    memset(&frame, 0, sizeof(frame));
    frame.pixels = rgba;
    frame.width  = (uint32_t)width;
    frame.height = (uint32_t)height;
    frame.stride = (uint32_t)width * 4u;
    (void)dmcp_screenshot_submit(ctx, &frame);
  }

  free(rgba);
  free(pixels);
}
