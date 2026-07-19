from contract_helpers import check, read_repo_text


DASHBOARD_APP = "src/firmware/bikemb/src/app/dashboard_app.cpp"
DASHBOARD_CORE_HEADER = "src/firmware/bikemb/src/app/dashboard_view_core.h"
DASHBOARD_PAGES = "src/firmware/bikemb/src/app/dashboard_pages.c"
AI_UI_STATE = "src/firmware/bikemb/src/app/ai_assistant_ui_state.cpp"


def test_dashboard_renders_dedicated_ai_page_from_snapshot_state() -> None:
    app = read_repo_text(DASHBOARD_APP)
    core_header = read_repo_text(DASHBOARD_CORE_HEADER)
    pages = read_repo_text(DASHBOARD_PAGES)
    ai_ui_state = read_repo_text(AI_UI_STATE)

    check("BikeMbAiAssistant_GetSnapshot" in app, "Dashboard app should read the AI snapshot once per UI update.")
    check("BikeMbAiUiState_FromSnapshot" in app, "Dashboard app should map snapshots through the UI state adapter.")
    check("BikeMbDashboardAiUiState ai" in core_header, "Dashboard metrics should carry a UI-facing AI state.")
    check("ai_state_label" in pages, "Dashboard pages should own a dedicated AI state label.")
    check("ai_action_hint" in pages, "Dashboard pages should render the AI action hint.")
    check("ai_network" in pages, "AI page should show cloud/network summary.")
    check('"Tap to talk"' in ai_ui_state, "Idle AI UI state should use the design text.")
    check('"Listening"' in ai_ui_state, "Recording AI UI state should render as Listening.")
    check('"Failed"' in ai_ui_state, "Error AI UI state should render as Failed.")


def test_dashboard_ai_page_does_not_call_backend_services_directly() -> None:
    pages = read_repo_text(DASHBOARD_PAGES)

    check("cloud_worker" not in pages, "AI page must not call cloud worker APIs directly.")
    check("audio_" not in pages.lower(), "AI page must not call audio APIs directly.")
    check("BikeMbAiAssistant_" not in pages, "AI page must render only the UI-facing snapshot state.")


if __name__ == "__main__":
    test_dashboard_renders_dedicated_ai_page_from_snapshot_state()
    test_dashboard_ai_page_does_not_call_backend_services_directly()
    print("PASS test_dashboard_ai_ui_contract")
