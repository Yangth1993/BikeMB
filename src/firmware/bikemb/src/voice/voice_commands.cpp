#include "voice_commands.h"

#ifndef BIKE_MB_ENABLE_VOICE_COMMANDS
#define BIKE_MB_ENABLE_VOICE_COMMANDS 0
#endif

#if defined(ARDUINO) && !defined(BIKE_MB_USE_ESPIDF_RUNTIME) && BIKE_MB_ENABLE_VOICE_COMMANDS

#include <Arduino.h>
#include "ESP_I2S.h"
#include "ESP_SR.h"

#include "../drivers/I2C_Driver.h"

namespace {

enum {
  kSrCommandNextPage = 1,
  kSrCommandPreviousPage = 2,
};

constexpr uint8_t kEs7210Address = 0x40;
constexpr uint32_t kSampleRate = 16000;
constexpr const char *kSrInputFormat = "MN";

I2SClass g_i2s(I2S_NUM_0);
BikeMbVoiceCommand g_pendingCommand = BIKE_MB_VOICE_COMMAND_NONE;

static const sr_cmd_t kSrCommands[] = {
    {kSrCommandNextPage, "Next page"},
    {kSrCommandNextPage, "Next screen"},
    {kSrCommandNextPage, "Next song"},
    {kSrCommandPreviousPage, "Previous page"},
    {kSrCommandPreviousPage, "Previous screen"},
    {kSrCommandPreviousPage, "Go back"},
};

bool writeRegister(uint8_t reg, uint8_t value) {
  return !I2C_Write(kEs7210Address, reg, &value, 1);
}

bool initMicrophoneCodec() {
  bool ok = true;
  ok &= writeRegister(0x00, 0xFF);
  ok &= writeRegister(0x00, 0x32);
  ok &= writeRegister(0x09, 0x30);
  ok &= writeRegister(0x0A, 0x30);
  ok &= writeRegister(0x23, 0x2A);
  ok &= writeRegister(0x22, 0x0A);
  ok &= writeRegister(0x21, 0x2A);
  ok &= writeRegister(0x20, 0x0A);
  ok &= writeRegister(0x11, 0x60);
  ok &= writeRegister(0x12, 0x00);
  ok &= writeRegister(0x40, 0xC3);
  ok &= writeRegister(0x41, 0x70);
  ok &= writeRegister(0x42, 0x70);
  ok &= writeRegister(0x43, 0x1D);
  ok &= writeRegister(0x44, 0x1D);
  ok &= writeRegister(0x45, 0x1D);
  ok &= writeRegister(0x46, 0x1D);
  ok &= writeRegister(0x47, 0x08);
  ok &= writeRegister(0x48, 0x08);
  ok &= writeRegister(0x49, 0x08);
  ok &= writeRegister(0x4A, 0x08);
  ok &= writeRegister(0x07, 0x20);
  ok &= writeRegister(0x02, 0xC1);
  ok &= writeRegister(0x04, 0x01);
  ok &= writeRegister(0x05, 0x00);
  ok &= writeRegister(0x06, 0x04);
  ok &= writeRegister(0x4B, 0x0F);
  ok &= writeRegister(0x4C, 0x0F);
  ok &= writeRegister(0x00, 0x71);
  ok &= writeRegister(0x00, 0x41);
  ok &= writeRegister(0x1B, 0xFF);
  ok &= writeRegister(0x1C, 0xFF);
  ok &= writeRegister(0x1D, 0xFF);
  ok &= writeRegister(0x1E, 0xFF);
  return ok;
}

void handleSrEvent(sr_event_t event, int commandId, int phraseId) {
  (void)phraseId;
  if (event == SR_EVENT_TIMEOUT) {
    ESP_SR.setMode(SR_MODE_COMMAND);
    Serial.println("[BikeMB][voice] command timeout");
    return;
  }
  if (event != SR_EVENT_COMMAND) {
    return;
  }

  if (commandId == kSrCommandNextPage) {
    g_pendingCommand = BIKE_MB_VOICE_COMMAND_NEXT_PAGE;
    Serial.println("[BikeMB][voice] recognized: next page");
  } else if (commandId == kSrCommandPreviousPage) {
    g_pendingCommand = BIKE_MB_VOICE_COMMAND_PREVIOUS_PAGE;
    Serial.println("[BikeMB][voice] recognized: previous page");
  } else {
    Serial.printf("[BikeMB][voice] unknown command id=%d\n", commandId);
  }
  ESP_SR.setMode(SR_MODE_COMMAND);
}

}  // namespace

void BikeMbVoiceCommands_Init(void) {
  Serial.println("[BikeMB][voice] direct command mode enabled");

  if (!initMicrophoneCodec()) {
    Serial.println("[BikeMB][voice] ES7210 init failed");
    return;
  }

  g_i2s.setPins(GPIO_NUM_48, GPIO_NUM_38, GPIO_NUM_47, GPIO_NUM_39, GPIO_NUM_2);
  g_i2s.setTimeout(1000);
  if (!g_i2s.begin(I2S_MODE_STD, kSampleRate, I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO)) {
    Serial.println("[BikeMB][voice] I2S init failed");
    return;
  }

  ESP_SR.onEvent(handleSrEvent);
  if (!ESP_SR.begin(g_i2s, kSrCommands, sizeof(kSrCommands) / sizeof(kSrCommands[0]), SR_CHANNELS_STEREO, SR_MODE_COMMAND, kSrInputFormat)) {
    Serial.println("[BikeMB][voice] ESP-SR init failed");
    return;
  }

  Serial.println("[BikeMB][voice] say: Next page / Previous page");
}

BikeMbVoiceCommand BikeMbVoiceCommands_ConsumeCommand(void) {
  const BikeMbVoiceCommand command = g_pendingCommand;
  g_pendingCommand = BIKE_MB_VOICE_COMMAND_NONE;
  return command;
}

#else

void BikeMbVoiceCommands_Init(void) {}
BikeMbVoiceCommand BikeMbVoiceCommands_ConsumeCommand(void) {
  return BIKE_MB_VOICE_COMMAND_NONE;
}

#endif
