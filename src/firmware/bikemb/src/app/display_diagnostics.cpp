#include "display_diagnostics.h"

#include "esp_heap_caps.h"

#include "../drivers/Display_ST77916.h"
#include "platform/bike_platform.h"

namespace {

constexpr uint16_t kWidth = EXAMPLE_LCD_WIDTH;
constexpr uint16_t kHeight = EXAMPLE_LCD_HEIGHT;
constexpr uint16_t kStripeHeight = 30;
constexpr size_t kBufferPixels = static_cast<size_t>(kWidth) * kStripeHeight;

uint16_t *g_buffer = nullptr;

uint16_t ToPanelColor(uint16_t rgb565) {
  return static_cast<uint16_t>((rgb565 >> 8) | (rgb565 << 8));
}

void FillBuffer(uint16_t rgb565, size_t pixels) {
  const uint16_t panelColor = ToPanelColor(rgb565);
  for (size_t i = 0; i < pixels; ++i) {
    g_buffer[i] = panelColor;
  }
}

void FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t rgb565) {
  uint16_t row = y;
  uint16_t remaining = h;
  while (remaining > 0) {
    const uint16_t chunkHeight = remaining > kStripeHeight ? kStripeHeight : remaining;
    const size_t pixels = static_cast<size_t>(w) * chunkHeight;
    FillBuffer(rgb565, pixels);
    LCD_addWindow(x, row, x + w - 1, row + chunkHeight - 1, g_buffer);
    row += chunkHeight;
    remaining -= chunkHeight;
    BikePlatform_DelayMs(10);
  }
}

}  // namespace

void DisplayDiagnostics_Run() {
  BikePlatform_LogInfo("BikeMB.DisplayDiagnostic", "display diagnostic start");

  g_buffer = static_cast<uint16_t *>(
      heap_caps_malloc(kBufferPixels * sizeof(uint16_t), MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL));
  if (g_buffer == nullptr) {
    BikePlatform_LogInfo("BikeMB.DisplayDiagnostic", "display diagnostic buffer allocation failed");
    return;
  }

  FillRect(0, 0, kWidth, kHeight, 0x0000);
  BikePlatform_DelayMs(300);

  FillRect(0, 0, 180, 180, 0xF800);
  FillRect(180, 0, 180, 180, 0x07E0);
  FillRect(0, 180, 180, 180, 0x001F);
  FillRect(180, 180, 180, 180, 0xFFFF);
  BikePlatform_DelayMs(800);

  const uint16_t colors[] = {
      0xF800, 0xFFE0, 0x07E0, 0x07FF, 0x001F, 0xF81F,
      0xFFFF, 0x8410, 0x4208, 0x0000, 0xFBE0, 0x041F,
  };
  const uint16_t bandHeight = kHeight / (sizeof(colors) / sizeof(colors[0]));
  for (size_t i = 0; i < sizeof(colors) / sizeof(colors[0]); ++i) {
    FillRect(0, static_cast<uint16_t>(i * bandHeight), kWidth, bandHeight, colors[i]);
  }

  BikePlatform_LogInfo("BikeMB.DisplayDiagnostic", "display diagnostic ready");
}
