from contract_helpers import check, find_function_body, read_repo_text


PAGES_HEADER = "src/firmware/bikemb/src/app/dashboard_pages.h"
PAGES_SOURCE = "src/firmware/bikemb/src/app/dashboard_pages.c"
STYLE_HEADER = "src/firmware/bikemb/src/app/dashboard_ui_style.h"
STYLE_SOURCE = "src/firmware/bikemb/src/app/dashboard_ui_style.c"
VIEW_CORE = "src/firmware/bikemb/src/app/dashboard_view_core.c"
SIMULATOR_UI = "tools/simulator/bikemb_ui/bikemb_dashboard.c"
DESIGN_DOC = "docs/ui-ux/ui-redesign-avinox-ux.md"


def test_mode_color_tokens_exist() -> None:
    header = read_repo_text(STYLE_HEADER)
    source = read_repo_text(STYLE_SOURCE)

    for name in (
        "BikeMbUi_ColorEco",
        "BikeMbUi_ColorTrail",
        "BikeMbUi_ColorAuto",
        "BikeMbUi_ColorBoost",
        "BikeMbUi_ModeColor",
    ):
        check(name in header, f"{name} must be exposed for mode-color UI styling.")
        check(name in source, f"{name} must be implemented for mode-color UI styling.")

    check("35, 214, 107" in source, "ECO must use the confirmed green accent.")
    check("243, 181, 63" in source, "Trail must use the confirmed yellow/amber accent.")
    check("35, 140, 255" in source, "AUTO must use the confirmed blue accent.")
    check("240, 78, 62" in source, "Boost must use the confirmed red accent.")


def test_dashboard_uses_avinox_single_mode_color() -> None:
    header = read_repo_text(PAGES_HEADER)
    source = read_repo_text(PAGES_SOURCE)

    check("home_speed_major" in header, "Home speed must split the large integer digits from the decimal digit.")
    check("home_speed_decimal" in header, "Home speed must keep a separate decimal label for Avinox hierarchy.")
    check("home_assist_glow" in header, "Home assist visualization must retain its generated segmented artwork.")
    check("ai_state_label" in header, "AI page must keep a dedicated state label.")
    check("ai_action_hint" in header, "AI page must keep a dedicated action hint.")
    check("detail_icons" in header, "Detail page icons must be recolored with the active mode color.")
    check("BIKE_MB_DASHBOARD_MODE_COUNT = 4" in header, "Dashboard must support ECO/TRAIL/AUTO/BOOST.")

    for label in ("ECO", "TRAIL", "AUTO", "BOOST"):
        check(f'"{label}"' in source, f"Mode label {label} must be present.")

    check("BikeMbUi_ModeColor(pages->home_mode_index)" in source, "Page accents must derive from the active mode.")
    check("LV_SYMBOL_BATTERY_FULL" in source, "Dashboard battery status must use the LVGL Font Awesome icon.")
    check("bike_mb_img_home_bezel" in source, "Home page must use an independent neutral bezel asset.")
    check("bike_mb_img_home_assist_glow" in source, "Home assist visualization must use the generated segmented image.")
    check("lv_obj_set_style_img_recolor(pages->home_assist_glow" in source,
          "Mode updates must recolor the generated assist artwork.")
    check("lv_obj_set_style_line_color(pages->wave_line" in source, "AI waveform line must recolor by state.")


def test_home_time_is_prominent() -> None:
    source = read_repo_text(PAGES_SOURCE)
    body = find_function_body(source, "static void create_home_page")

    check("pages->home_time" in body, "Home page must keep an explicit time label.")
    check("&lv_font_montserrat_48" in body,
          "Home speed must use the hardware-verified built-in font until custom-font compatibility is resolved.")
    check("&bike_mb_font_speed_140" not in body,
          "Home speed must not use the custom font that disappears on ESP32-S3 hardware.")
    check("&lv_font_montserrat_22" in body or "&lv_font_montserrat_26" in body,
          "Home time must use a larger font than the old 18 px label.")
    check('"TIME"' not in body, "Home time must match the reference without a redundant TIME caption.")


def test_ai_page_matches_selected_design_structure() -> None:
    source = read_repo_text(PAGES_SOURCE)
    body = find_function_body(source, "static void create_ai_page")

    check("create_dashboard_bezel" in body, "AI page must reuse the reference metal bezel asset.")
    check("pages->ai_ring = lv_arc_create" in body, "AI page must render the selected center status ring.")
    check("pages->wave_line = lv_line_create" in body, "AI page must render the selected waveform language.")
    check('"AI"' in body, "AI page must show the compact AI identity label.")
    check('"Offline"' in body, "AI page must have a safe offline default before snapshots arrive.")


def test_settings_ux_is_implemented() -> None:
    header = read_repo_text(PAGES_HEADER)
    source = read_repo_text(PAGES_SOURCE)
    core = read_repo_text(VIEW_CORE)

    for field in ("settings_page", "accessories_page", "about_page", "last_dashboard_page"):
        check(field in header, f"{field} must be tracked in the dashboard view state.")

    for text in ("SETTINGS", "ACCESSORIES", "ABOUT DEVICE", "BikeMB", "v0.1.0", "No accessories connected"):
        check(f'"{text}"' in source, f"{text} must appear in the settings UX.")

    check("CST816_GESTURE_SWIPE_UP" in core, "Swipe up must enter settings from dashboard.")
    check("CST816_GESTURE_SWIPE_DOWN" in core, "Swipe down must return from settings/detail pages.")
    check("BikeMbDashboardPages_ShowSettings" in core, "Core gesture handling must use the settings entry API.")
    check("BikeMbDashboardPages_ReturnFromSettings" in core, "Core gesture handling must use the settings return API.")
    check("BikeMbDashboardPages_ShowDashboardPage" in header, "Pages API must allow returning to the previous dashboard page.")


def test_simulator_uses_current_dashboard_metrics() -> None:
    source = read_repo_text(SIMULATOR_UI)

    for field in ("uptimeMs", "rideSeconds", "speedKmh", "tripKm", "assistPowerW", "batteryPercent", "wavePhase"):
        check(f".{field}" in source, f"Simulator metrics must populate {field} for the redesigned dashboard.")


def test_design_doc_is_linked_to_implementation_terms() -> None:
    doc = read_repo_text(DESIGN_DOC)

    for term in ("ECO", "Trail", "AUTO", "Boost", "配件连接", "关于设备", "swipe up", "BIKE_MB_FIRMWARE_VERSION"):
        check(term in doc, f"Design doc must retain implementation term: {term}")


if __name__ == "__main__":
    test_mode_color_tokens_exist()
    test_dashboard_uses_avinox_single_mode_color()
    test_home_time_is_prominent()
    test_ai_page_matches_selected_design_structure()
    test_settings_ux_is_implemented()
    test_simulator_uses_current_dashboard_metrics()
    test_design_doc_is_linked_to_implementation_terms()
    print("PASS test_avinox_ui_contract")
