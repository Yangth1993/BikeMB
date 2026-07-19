#pragma once

#include <stdbool.h>
#include <stdint.h>

enum BikeMbWifiServiceAction : uint32_t {
  BIKE_MB_WIFI_SERVICE_ACTION_NONE = 0,
  BIKE_MB_WIFI_SERVICE_ACTION_START_CONNECT = 1U << 0,
  BIKE_MB_WIFI_SERVICE_ACTION_PUBLISH_CONNECTED = 1U << 1,
  BIKE_MB_WIFI_SERVICE_ACTION_PUBLISH_DISCONNECTED = 1U << 2,
};

struct BikeMbWifiServiceCore {
  bool enabled;
  bool configured;
  bool connected;
  bool connecting;
  bool published;
  uint32_t lastConnectAttemptMs;
  uint32_t retryIntervalMs;
};

void BikeMbWifiServiceCore_Init(
    BikeMbWifiServiceCore *core,
    bool enabled,
    bool configured,
    uint32_t nowMs);
uint32_t BikeMbWifiServiceCore_Update(
    BikeMbWifiServiceCore *core,
    uint32_t nowMs,
    bool connected);
