#include <assert.h>

#include "../../src/firmware/bikemb/src/network/wifi_service_core.h"

static uint32_t update(
    BikeMbWifiServiceCore *core,
    uint32_t nowMs,
    bool connected) {
  return BikeMbWifiServiceCore_Update(core, nowMs, connected);
}

int main() {
  BikeMbWifiServiceCore disabled = {};
  BikeMbWifiServiceCore_Init(&disabled, false, true, 100);
  assert(update(&disabled, 100, false) == BIKE_MB_WIFI_SERVICE_ACTION_NONE);
  assert(!disabled.connecting);

  BikeMbWifiServiceCore missingConfig = {};
  BikeMbWifiServiceCore_Init(&missingConfig, true, false, 100);
  assert(update(&missingConfig, 100, false) ==
         BIKE_MB_WIFI_SERVICE_ACTION_PUBLISH_DISCONNECTED);
  assert(update(&missingConfig, 2000, false) == BIKE_MB_WIFI_SERVICE_ACTION_NONE);

  BikeMbWifiServiceCore core = {};
  BikeMbWifiServiceCore_Init(&core, true, true, 1000);
  assert(update(&core, 1000, false) ==
         (BIKE_MB_WIFI_SERVICE_ACTION_START_CONNECT |
          BIKE_MB_WIFI_SERVICE_ACTION_PUBLISH_DISCONNECTED));
  assert(core.connecting);
  assert(core.lastConnectAttemptMs == 1000);

  assert(update(&core, 2000, false) == BIKE_MB_WIFI_SERVICE_ACTION_NONE);
  assert(update(&core, 11000, false) == BIKE_MB_WIFI_SERVICE_ACTION_START_CONNECT);

  assert(update(&core, 11100, true) ==
         BIKE_MB_WIFI_SERVICE_ACTION_PUBLISH_CONNECTED);
  assert(core.connected);
  assert(!core.connecting);

  assert(update(&core, 11200, true) == BIKE_MB_WIFI_SERVICE_ACTION_NONE);
  assert(update(&core, 12000, false) ==
         (BIKE_MB_WIFI_SERVICE_ACTION_START_CONNECT |
          BIKE_MB_WIFI_SERVICE_ACTION_PUBLISH_DISCONNECTED));
  return 0;
}
