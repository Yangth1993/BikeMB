#include "ai_assistant_ui_state.h"

namespace {

BikeMbDashboardAiUiState make_state(BikeMbDashboardAiVisualState visual_state,
                                    BikeMbDashboardAiSurface preferred_surface,
                                    uint8_t battery_percent,
                                    uint8_t can_cancel,
                                    uint8_t can_retry,
                                    const char *network_text,
                                    const char *state_text,
                                    const char *action_hint) {
  BikeMbDashboardAiUiState state{};
  state.visual_state = visual_state;
  state.preferred_surface = preferred_surface;
  state.battery_percent = battery_percent;
  state.can_cancel = can_cancel;
  state.can_retry = can_retry;
  state.network_text = network_text;
  state.state_text = state_text;
  state.action_hint = action_hint;
  return state;
}

const char *network_text_for(const BikeMbAiSnapshot &snapshot) {
  return snapshot.wifiConnected ? "Cloud" : "Offline";
}

}  // namespace

BikeMbDashboardAiUiState BikeMbAiUiState_FromSnapshot(const BikeMbAiSnapshot &snapshot,
                                                      uint8_t battery_percent) {
  if (snapshot.state == BIKE_MB_AI_DISABLED || !snapshot.wifiConnected) {
    return make_state(BIKE_MB_DASHBOARD_AI_VISUAL_OFFLINE,
                      BIKE_MB_DASHBOARD_AI_SURFACE_FULL_PAGE,
                      battery_percent,
                      0,
                      0,
                      "Offline",
                      "Offline",
                      "AI unavailable");
  }

  const char *network_text = network_text_for(snapshot);
  switch (snapshot.state) {
    case BIKE_MB_AI_IDLE:
      return make_state(BIKE_MB_DASHBOARD_AI_VISUAL_IDLE,
                        BIKE_MB_DASHBOARD_AI_SURFACE_FULL_PAGE,
                        battery_percent,
                        0,
                        0,
                        network_text,
                        "Tap to talk",
                        "Center to start");
    case BIKE_MB_AI_RECORDING:
      return make_state(BIKE_MB_DASHBOARD_AI_VISUAL_LISTENING,
                        BIKE_MB_DASHBOARD_AI_SURFACE_MINI_OVERLAY,
                        battery_percent,
                        1,
                        0,
                        network_text,
                        "Listening",
                        "Press to cancel");
    case BIKE_MB_AI_RECOGNIZING:
      return make_state(BIKE_MB_DASHBOARD_AI_VISUAL_SENDING,
                        BIKE_MB_DASHBOARD_AI_SURFACE_MINI_OVERLAY,
                        battery_percent,
                        snapshot.cancelAvailable ? 1 : 0,
                        0,
                        network_text,
                        "Sending",
                        "Press to cancel");
    case BIKE_MB_AI_THINKING:
    case BIKE_MB_AI_SYNTHESIZING:
      return make_state(BIKE_MB_DASHBOARD_AI_VISUAL_THINKING,
                        BIKE_MB_DASHBOARD_AI_SURFACE_MINI_OVERLAY,
                        battery_percent,
                        snapshot.cancelAvailable ? 1 : 0,
                        0,
                        network_text,
                        "Thinking",
                        "Press to cancel");
    case BIKE_MB_AI_SPEAKING:
      return make_state(BIKE_MB_DASHBOARD_AI_VISUAL_SPEAKING,
                        BIKE_MB_DASHBOARD_AI_SURFACE_FULL_PAGE,
                        battery_percent,
                        1,
                        0,
                        network_text,
                        "Speaking",
                        "Press to stop");
    case BIKE_MB_AI_CONNECTING_MUSIC:
    case BIKE_MB_AI_MUSIC_PLAYING:
      return make_state(BIKE_MB_DASHBOARD_AI_VISUAL_MUSIC,
                        BIKE_MB_DASHBOARD_AI_SURFACE_FULL_PAGE,
                        battery_percent,
                        1,
                        0,
                        network_text,
                        "Music",
                        "Press to stop");
    case BIKE_MB_AI_ERROR:
      return make_state(BIKE_MB_DASHBOARD_AI_VISUAL_ERROR,
                        BIKE_MB_DASHBOARD_AI_SURFACE_CHIP,
                        battery_percent,
                        0,
                        1,
                        network_text,
                        "Failed",
                        "Press to clear");
    case BIKE_MB_AI_DISABLED:
    default:
      return make_state(BIKE_MB_DASHBOARD_AI_VISUAL_OFFLINE,
                        BIKE_MB_DASHBOARD_AI_SURFACE_FULL_PAGE,
                        battery_percent,
                        0,
                        0,
                        "Offline",
                        "Offline",
                        "AI unavailable");
  }
}
