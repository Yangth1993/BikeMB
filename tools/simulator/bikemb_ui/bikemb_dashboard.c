#include "bikemb_dashboard.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <SDL2/SDL.h>
#include "dashboard_view_core.h"
#include "lvgl/lvgl.h"

enum {
  kOrbSize = 48,
  kOrbMinX = 26,
  kOrbMaxX = 360 - kOrbSize - 26,
  kOrbMinY = 198,
  kOrbMaxY = 360 - kOrbSize - 62,
};

static int16_t g_orb_x = 52;
static int16_t g_orb_y = 206;
static int8_t g_velocity_x = 3;
static int8_t g_velocity_y = 2;
static uint32_t g_frame_count;
static uint32_t g_last_fps_at;
static float g_fps;
static float g_cpu_load;

static SDL_Window *find_simulator_window(void) {
  for (uint32_t id = 1; id < 32; ++id) {
    SDL_Window *window = SDL_GetWindowFromID(id);
    if (window != NULL && strcmp(SDL_GetWindowTitle(window), "TFT Simulator") == 0) {
      return window;
    }
  }
  return NULL;
}

static void capture_dashboard(lv_timer_t *timer) {
  const char *path = getenv("BIKEMB_SIMULATOR_CAPTURE_PATH");
  if (path == NULL || path[0] == '\0') {
    lv_timer_del(timer);
    return;
  }

  SDL_Window *window = find_simulator_window();
  SDL_Renderer *renderer = window != NULL ? SDL_GetRenderer(window) : NULL;
  int width = 0;
  int height = 0;
  if (renderer == NULL || SDL_GetRendererOutputSize(renderer, &width, &height) != 0) {
    fprintf(stderr, "BikeMB simulator capture failed: %s\n", SDL_GetError());
    lv_timer_del(timer);
    return;
  }

  const int pitch = width * 4;
  void *pixels = malloc((size_t)pitch * (size_t)height);
  if (pixels == NULL || SDL_RenderReadPixels(renderer, NULL, SDL_PIXELFORMAT_ARGB8888, pixels, pitch) != 0) {
    fprintf(stderr, "BikeMB simulator capture failed: %s\n", pixels == NULL ? "out of memory" : SDL_GetError());
    free(pixels);
    lv_timer_del(timer);
    return;
  }

  SDL_Surface *surface = SDL_CreateRGBSurfaceWithFormatFrom(
      pixels, width, height, 32, pitch, SDL_PIXELFORMAT_ARGB8888);
  if (surface == NULL || SDL_SaveBMP(surface, path) != 0) {
    fprintf(stderr, "BikeMB simulator capture failed: %s\n", SDL_GetError());
  } else {
    printf("BikeMB simulator capture saved: %s\n", path);
  }

  SDL_FreeSurface(surface);
  free(pixels);
  lv_timer_del(timer);
}

static void update_orb(void) {
  g_orb_x += g_velocity_x;
  g_orb_y += g_velocity_y;

  if (g_orb_x <= kOrbMinX || g_orb_x >= kOrbMaxX) {
    g_velocity_x = (int8_t)-g_velocity_x;
    g_orb_x += g_velocity_x;
  }

  if (g_orb_y <= kOrbMinY || g_orb_y >= kOrbMaxY) {
    g_velocity_y = (int8_t)-g_velocity_y;
    g_orb_y += g_velocity_y;
  }
}

static void update_dashboard(lv_timer_t *timer) {
  (void)timer;

  ++g_frame_count;
  update_orb();

  const uint32_t now = lv_tick_get();
  if (g_last_fps_at == 0) {
    g_last_fps_at = now;
  }

  if (now - g_last_fps_at >= 1000) {
    g_fps = g_frame_count * 1000.0f / (float)(now - g_last_fps_at);
    g_frame_count = 0;
    g_last_fps_at = now;
  }

  const uint32_t phase = (now / 33U) % 200U;
  const float wave = phase <= 100U ? (float)phase : (float)(200U - phase);
  g_cpu_load = 0.88f * g_cpu_load + 0.12f * (35.0f + wave * 0.55f);

  const int mem_load = 42 + (int)((now / 180U) % 28U);
  const int psram_load = 18 + (int)((now / 260U) % 18U);
  const int capture_state = getenv("BIKEMB_SIMULATOR_CAPTURE_PATH") != NULL;

  BikeMbDashboardMetrics metrics = {
      .fps = g_fps,
      .cpuLoad = g_cpu_load,
      .heapFree = (uint32_t)(100 - mem_load),
      .heapTotal = 100,
      .psramFree = (uint32_t)(100 - psram_load),
      .psramTotal = 100,
      .orbX = g_orb_x,
      .orbY = g_orb_y,
      .uptimeMs = capture_state ? 45000000U : now,
      .rideSeconds = now / 1000U,
      .speedKmh = capture_state ? 24.5f : 24.5f + (float)((now / 140U) % 8U) * 0.1f,
      .tripKm = capture_state ? 0.0f : 17.2f + (float)((now / 5000U) % 20U) * 0.1f,
      .totalKm = 495.0f,
      .averageSpeedKmh = 21.4f,
      .assistPowerW = 186.0f + (float)((now / 160U) % 30U),
      .temperatureC = 18.0f,
      .gradePercent = 4.0f,
      .batteryPercent = 96,
      .wavePhase = (uint8_t)((now / 220U) % 24U),
  };
  BikeMbDashboardView_Update(&metrics);
}

void bikemb_dashboard_create(void) {
  BikeMbDashboardView_Create();
  const char *capture_page = getenv("BIKEMB_SIMULATOR_CAPTURE_PAGE");
  const int page_index = capture_page != NULL ? atoi(capture_page) : 0;
  for (int page = 0; page < page_index && page < 3; ++page) {
    BikeMbDashboardView_NextPage();
  }
  lv_timer_create(update_dashboard, 33, NULL);
  lv_timer_create(capture_dashboard, 1000, NULL);
}
