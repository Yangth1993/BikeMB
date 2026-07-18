#include <Arduino.h>
#include "esp_heap_caps.h"
#include "esp_timer.h"

#include "Display_ST77916.h"
#include "I2C_Driver.h"
#include "TCA9554PWR.h"

namespace {
constexpr uint16_t kWidth = EXAMPLE_LCD_WIDTH;
constexpr uint16_t kHeight = EXAMPLE_LCD_HEIGHT;
constexpr uint16_t kSpriteSize = 48;
constexpr uint32_t kTargetFrameMs = 33;

uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

const uint16_t kBg = rgb565(7, 13, 24);
const uint16_t kPanel = rgb565(16, 27, 42);
const uint16_t kText = rgb565(238, 244, 248);
const uint16_t kMuted = rgb565(118, 139, 156);
const uint16_t kGreen = rgb565(42, 210, 142);
const uint16_t kAmber = rgb565(255, 187, 68);
const uint16_t kCyan = rgb565(51, 190, 235);
const uint16_t kRed = rgb565(237, 84, 89);
const uint16_t kWhite = rgb565(255, 255, 255);
const uint16_t kBlack = rgb565(0, 0, 0);

int16_t spriteX = 52;
int16_t spriteY = 206;
int16_t lastSpriteX = spriteX;
int16_t lastSpriteY = spriteY;
int8_t velocityX = 3;
int8_t velocityY = 2;
uint32_t frameCount = 0;
uint32_t lastFpsAt = 0;
float fps = 0.0f;
float cpuLoad = 0.0f;

void drawRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
  if (x >= kWidth || y >= kHeight || w == 0 || h == 0) {
    return;
  }

  if (x + w > kWidth) {
    w = kWidth - x;
  }
  if (y + h > kHeight) {
    h = kHeight - y;
  }

  const size_t pixels = static_cast<size_t>(w) * h;
  uint16_t *buffer = static_cast<uint16_t *>(
      heap_caps_malloc(pixels * sizeof(uint16_t), MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL));
  if (buffer == nullptr) {
    buffer = static_cast<uint16_t *>(heap_caps_malloc(pixels * sizeof(uint16_t), MALLOC_CAP_8BIT));
  }
  if (buffer == nullptr) {
    Serial.println("[BikeMB] drawRect skipped: no framebuffer memory");
    return;
  }

  for (size_t i = 0; i < pixels; ++i) {
    buffer[i] = color;
  }
  LCD_addWindow(x, y, x + w - 1, y + h - 1, buffer);
  free(buffer);
}

const uint8_t *glyphFor(char c) {
  static const uint8_t space[7] = {0, 0, 0, 0, 0, 0, 0};
  static const uint8_t zero[7] = {0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E};
  static const uint8_t one[7] = {0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E};
  static const uint8_t two[7] = {0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F};
  static const uint8_t three[7] = {0x1E, 0x01, 0x01, 0x0E, 0x01, 0x01, 0x1E};
  static const uint8_t four[7] = {0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02};
  static const uint8_t five[7] = {0x1F, 0x10, 0x10, 0x1E, 0x01, 0x01, 0x1E};
  static const uint8_t six[7] = {0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E};
  static const uint8_t seven[7] = {0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08};
  static const uint8_t eight[7] = {0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E};
  static const uint8_t nine[7] = {0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x0C};
  static const uint8_t a[7] = {0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11};
  static const uint8_t b[7] = {0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E};
  static const uint8_t c_[7] = {0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E};
  static const uint8_t d[7] = {0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E};
  static const uint8_t e[7] = {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F};
  static const uint8_t f[7] = {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10};
  static const uint8_t h[7] = {0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11};
  static const uint8_t i[7] = {0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E};
  static const uint8_t k[7] = {0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11};
  static const uint8_t l[7] = {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F};
  static const uint8_t m[7] = {0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11};
  static const uint8_t p[7] = {0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10};
  static const uint8_t r[7] = {0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11};
  static const uint8_t s[7] = {0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E};
  static const uint8_t u[7] = {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E};
  static const uint8_t v[7] = {0x11, 0x11, 0x11, 0x11, 0x0A, 0x0A, 0x04};
  static const uint8_t colon[7] = {0, 0x04, 0x04, 0, 0x04, 0x04, 0};
  static const uint8_t percent[7] = {0x19, 0x19, 0x02, 0x04, 0x08, 0x13, 0x13};
  static const uint8_t dot[7] = {0, 0, 0, 0, 0, 0x0C, 0x0C};
  static const uint8_t slash[7] = {0x01, 0x01, 0x02, 0x04, 0x08, 0x10, 0x10};

  if (c >= 'a' && c <= 'z') {
    c -= 32;
  }
  switch (c) {
    case '0': return zero;
    case '1': return one;
    case '2': return two;
    case '3': return three;
    case '4': return four;
    case '5': return five;
    case '6': return six;
    case '7': return seven;
    case '8': return eight;
    case '9': return nine;
    case 'A': return a;
    case 'B': return b;
    case 'C': return c_;
    case 'D': return d;
    case 'E': return e;
    case 'F': return f;
    case 'H': return h;
    case 'I': return i;
    case 'K': return k;
    case 'L': return l;
    case 'M': return m;
    case 'P': return p;
    case 'R': return r;
    case 'S': return s;
    case 'U': return u;
    case 'V': return v;
    case ':': return colon;
    case '%': return percent;
    case '.': return dot;
    case '/': return slash;
    default: return space;
  }
}

void drawChar(uint16_t x, uint16_t y, char c, uint16_t color, uint8_t scale) {
  const uint8_t *rows = glyphFor(c);
  for (uint8_t row = 0; row < 7; ++row) {
    for (uint8_t col = 0; col < 5; ++col) {
      if (rows[row] & (1 << (4 - col))) {
        drawRect(x + col * scale, y + row * scale, scale, scale, color);
      }
    }
  }
}

void drawText(uint16_t x, uint16_t y, const char *text, uint16_t color, uint8_t scale) {
  uint16_t cursor = x;
  while (*text) {
    drawChar(cursor, y, *text, color, scale);
    cursor += 6 * scale;
    ++text;
  }
}

void drawBar(uint16_t x, uint16_t y, uint16_t w, uint16_t h, float value, uint16_t color) {
  drawRect(x, y, w, h, rgb565(29, 43, 60));
  value = constrain(value, 0.0f, 100.0f);
  uint16_t fill = static_cast<uint16_t>((w - 4) * value / 100.0f);
  drawRect(x + 2, y + 2, fill, h - 4, color);
}

void drawSprite(uint16_t x, uint16_t y) {
  const size_t pixels = static_cast<size_t>(kSpriteSize) * kSpriteSize;
  uint16_t *buffer = static_cast<uint16_t *>(
      heap_caps_malloc(pixels * sizeof(uint16_t), MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL));
  if (buffer == nullptr) {
    Serial.println("[BikeMB] sprite skipped: no DMA memory");
    return;
  }

  for (uint16_t py = 0; py < kSpriteSize; ++py) {
    for (uint16_t px = 0; px < kSpriteSize; ++px) {
      const int16_t cx = px - 24;
      const int16_t cy = py - 24;
      uint16_t color = kPanel;

      if (cx * cx + cy * cy < 23 * 23) {
        color = ((px / 6 + py / 6) % 2 == 0) ? kCyan : rgb565(42, 69, 91);
      }
      if (cy > -5 && cy < 5 && cx > -18 && cx < 18) {
        color = kWhite;
      }
      if (cx > -5 && cx < 5 && cy > -18 && cy < 18) {
        color = kGreen;
      }
      if ((cx + 12) * (cx + 12) + (cy - 12) * (cy - 12) < 7 * 7 ||
          (cx - 12) * (cx - 12) + (cy - 12) * (cy - 12) < 7 * 7) {
        color = kBlack;
      }
      buffer[py * kSpriteSize + px] = color;
    }
  }
  LCD_addWindow(x, y, x + kSpriteSize - 1, y + kSpriteSize - 1, buffer);
  free(buffer);
}

void drawStaticLayout() {
  drawRect(0, 0, kWidth, kHeight, kBg);
  drawText(76, 24, "BIKEMB LIVE", kText, 3);
  drawText(84, 322, "MOVING IMAGE TEST", kMuted, 2);
  drawRect(30, 64, 300, 2, rgb565(38, 56, 74));
  drawRect(30, 304, 300, 2, rgb565(38, 56, 74));
}

void drawStats() {
  char line[32];
  const uint32_t heapTotal = ESP.getHeapSize();
  const uint32_t heapFree = ESP.getFreeHeap();
  const uint32_t psramTotal = ESP.getPsramSize();
  const uint32_t psramFree = ESP.getFreePsram();
  const float heapLoad = heapTotal == 0 ? 0.0f : 100.0f * (heapTotal - heapFree) / heapTotal;
  const float psramLoad = psramTotal == 0 ? 0.0f : 100.0f * (psramTotal - psramFree) / psramTotal;

  drawRect(42, 82, 276, 106, kPanel);

  snprintf(line, sizeof(line), "CPU:%3d%%", static_cast<int>(cpuLoad + 0.5f));
  drawText(58, 96, line, kText, 2);
  drawBar(172, 96, 116, 14, cpuLoad, cpuLoad > 75.0f ? kRed : kGreen);

  snprintf(line, sizeof(line), "MEM:%3d%%", static_cast<int>(heapLoad + 0.5f));
  drawText(58, 126, line, kText, 2);
  drawBar(172, 126, 116, 14, heapLoad, heapLoad > 75.0f ? kAmber : kCyan);

  snprintf(line, sizeof(line), "FPS:%4.1f", fps);
  drawText(58, 156, line, kText, 2);

  snprintf(line, sizeof(line), "PS:%3d%%", static_cast<int>(psramLoad + 0.5f));
  drawText(182, 156, line, kMuted, 2);
}

void updateSprite() {
  drawRect(lastSpriteX, lastSpriteY, kSpriteSize, kSpriteSize, kBg);

  spriteX += velocityX;
  spriteY += velocityY;
  if (spriteX <= 26 || spriteX >= static_cast<int16_t>(kWidth - kSpriteSize - 26)) {
    velocityX = -velocityX;
    spriteX += velocityX;
  }
  if (spriteY <= 198 || spriteY >= static_cast<int16_t>(kHeight - kSpriteSize - 62)) {
    velocityY = -velocityY;
    spriteY += velocityY;
  }

  drawSprite(spriteX, spriteY);
  lastSpriteX = spriteX;
  lastSpriteY = spriteY;
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(1500);

  Serial.println();
  Serial.println("[BikeMB] realtime dashboard bring-up");
  I2C_Init();
  TCA9554PWR_Init(0x00);
  Backlight_Init();
  Set_Backlight(80);
  LCD_Init();

  drawStaticLayout();
  drawStats();
  drawSprite(spriteX, spriteY);
  lastFpsAt = millis();

  Serial.println("[BikeMB] dashboard is running");
}

void loop() {
  const int64_t frameStartUs = esp_timer_get_time();

  updateSprite();
  ++frameCount;

  const uint32_t now = millis();
  if (now - lastFpsAt >= 1000) {
    fps = frameCount * 1000.0f / (now - lastFpsAt);
    frameCount = 0;
    lastFpsAt = now;
    drawStats();

    Serial.printf("[BikeMB] cpu=%2.0f%% fps=%4.1f heap=%u/%u psram=%u/%u\n",
                  cpuLoad,
                  fps,
                  ESP.getFreeHeap(),
                  ESP.getHeapSize(),
                  ESP.getFreePsram(),
                  ESP.getPsramSize());
  }

  const uint32_t frameMs = static_cast<uint32_t>((esp_timer_get_time() - frameStartUs) / 1000);
  cpuLoad = 0.85f * cpuLoad + 0.15f * min(100.0f, frameMs * 100.0f / kTargetFrameMs);

  if (frameMs < kTargetFrameMs) {
    delay(kTargetFrameMs - frameMs);
  } else {
    delay(1);
  }
}
