#include "wifi_service_core.h"

namespace {
constexpr uint32_t kRetryIntervalMs = 10000;

bool elapsed(uint32_t nowMs, uint32_t sinceMs, uint32_t intervalMs) {
  return static_cast<uint32_t>(nowMs - sinceMs) >= intervalMs;
}
}

void BikeMbWifiServiceCore_Init(
    BikeMbWifiServiceCore *core,
    bool enabled,
    bool configured,
    uint32_t nowMs) {
  if (core == nullptr) {
    return;
  }
  core->enabled = enabled;
  core->configured = configured;
  core->connected = false;
  core->connecting = false;
  core->published = false;
  core->lastConnectAttemptMs = nowMs;
  core->retryIntervalMs = kRetryIntervalMs;
}

uint32_t BikeMbWifiServiceCore_Update(
    BikeMbWifiServiceCore *core,
    uint32_t nowMs,
    bool connected) {
  if (core == nullptr || !core->enabled) {
    return BIKE_MB_WIFI_SERVICE_ACTION_NONE;
  }

  if (!core->configured) {
    core->connected = false;
    core->connecting = false;
    if (!core->published) {
      core->published = true;
      return BIKE_MB_WIFI_SERVICE_ACTION_PUBLISH_DISCONNECTED;
    }
    return BIKE_MB_WIFI_SERVICE_ACTION_NONE;
  }

  uint32_t actions = BIKE_MB_WIFI_SERVICE_ACTION_NONE;
  if (connected != core->connected) {
    core->connected = connected;
    core->connecting = false;
    core->published = true;
    actions |= connected ? BIKE_MB_WIFI_SERVICE_ACTION_PUBLISH_CONNECTED
                         : BIKE_MB_WIFI_SERVICE_ACTION_PUBLISH_DISCONNECTED;
  } else if (!core->published) {
    core->published = true;
    actions |= connected ? BIKE_MB_WIFI_SERVICE_ACTION_PUBLISH_CONNECTED
                         : BIKE_MB_WIFI_SERVICE_ACTION_PUBLISH_DISCONNECTED;
  }

  if (!connected &&
      (!core->connecting ||
       elapsed(nowMs, core->lastConnectAttemptMs, core->retryIntervalMs))) {
    core->connecting = true;
    core->lastConnectAttemptMs = nowMs;
    actions |= BIKE_MB_WIFI_SERVICE_ACTION_START_CONNECT;
  }

  return actions;
}
