#include <cassert>
#include <cstring>

#include "../../src/firmware/bikemb/src/app/ai_assistant_ui_state.h"

static BikeMbAiSnapshot snapshot(BikeMbAiState state, bool wifi, bool can_cancel) {
  BikeMbAiSnapshot value = {};
  value.state = state;
  value.wifiConnected = wifi;
  value.cancelAvailable = can_cancel;
  return value;
}

int main() {
  BikeMbDashboardAiUiState ui =
      BikeMbAiUiState_FromSnapshot(snapshot(BIKE_MB_AI_DISABLED, false, false), 87);
  assert(ui.visual_state == BIKE_MB_DASHBOARD_AI_VISUAL_OFFLINE);
  assert(ui.preferred_surface == BIKE_MB_DASHBOARD_AI_SURFACE_FULL_PAGE);
  assert(ui.battery_percent == 87);
  assert(std::strcmp(ui.network_text, "Offline") == 0);
  assert(std::strcmp(ui.state_text, "Offline") == 0);
  assert(std::strcmp(ui.action_hint, "AI unavailable") == 0);

  ui = BikeMbAiUiState_FromSnapshot(snapshot(BIKE_MB_AI_IDLE, true, false), 96);
  assert(ui.visual_state == BIKE_MB_DASHBOARD_AI_VISUAL_IDLE);
  assert(std::strcmp(ui.network_text, "Cloud") == 0);
  assert(std::strcmp(ui.state_text, "Tap to talk") == 0);

  ui = BikeMbAiUiState_FromSnapshot(snapshot(BIKE_MB_AI_RECORDING, true, true), 96);
  assert(ui.visual_state == BIKE_MB_DASHBOARD_AI_VISUAL_LISTENING);
  assert(ui.preferred_surface == BIKE_MB_DASHBOARD_AI_SURFACE_MINI_OVERLAY);
  assert(std::strcmp(ui.state_text, "Listening") == 0);
  assert(std::strcmp(ui.action_hint, "Press to cancel") == 0);

  ui = BikeMbAiUiState_FromSnapshot(snapshot(BIKE_MB_AI_RECOGNIZING, true, true), 96);
  assert(ui.visual_state == BIKE_MB_DASHBOARD_AI_VISUAL_SENDING);
  assert(std::strcmp(ui.state_text, "Sending") == 0);

  ui = BikeMbAiUiState_FromSnapshot(snapshot(BIKE_MB_AI_THINKING, true, true), 96);
  assert(ui.visual_state == BIKE_MB_DASHBOARD_AI_VISUAL_THINKING);
  assert(std::strcmp(ui.state_text, "Thinking") == 0);

  ui = BikeMbAiUiState_FromSnapshot(snapshot(BIKE_MB_AI_SPEAKING, true, true), 96);
  assert(ui.visual_state == BIKE_MB_DASHBOARD_AI_VISUAL_SPEAKING);
  assert(std::strcmp(ui.state_text, "Speaking") == 0);
  assert(std::strcmp(ui.action_hint, "Press to stop") == 0);

  ui = BikeMbAiUiState_FromSnapshot(snapshot(BIKE_MB_AI_MUSIC_PLAYING, true, true), 96);
  assert(ui.visual_state == BIKE_MB_DASHBOARD_AI_VISUAL_MUSIC);
  assert(std::strcmp(ui.state_text, "Music") == 0);

  ui = BikeMbAiUiState_FromSnapshot(snapshot(BIKE_MB_AI_ERROR, true, false), 96);
  assert(ui.visual_state == BIKE_MB_DASHBOARD_AI_VISUAL_ERROR);
  assert(std::strcmp(ui.state_text, "Failed") == 0);
  assert(std::strcmp(ui.action_hint, "Press to clear") == 0);

  return 0;
}
