from contract_helpers import check, read_repo_text


SETUP_SCRIPT = "tools/setup-lvgl-simulator.ps1"
OPEN_SCRIPT = "tools/open-lvgl-simulator.ps1"
SYNC_SCRIPT = "tools/sync-bikemb-simulator-ui.ps1"
BIKEMB_SIM_UI = "tools/simulator/bikemb_ui/bikemb_dashboard.c"
BIKEMB_SIM_UI_HEADER = "tools/simulator/bikemb_ui/bikemb_dashboard.h"
SHARED_DASHBOARD_CORE = "src/firmware/bikemb/src/app/dashboard_view_core.c"
SHARED_DASHBOARD_CORE_HEADER = "src/firmware/bikemb/src/app/dashboard_view_core.h"
SHARED_DASHBOARD_PAGES = "src/firmware/bikemb/src/app/dashboard_pages.c"
SHARED_DASHBOARD_STYLE = "src/firmware/bikemb/src/app/dashboard_ui_style.c"
FIRMWARE_DASHBOARD_WRAPPER = "src/firmware/bikemb/src/app/dashboard_view.cpp"
GITIGNORE = ".gitignore"


def test_simulator_setup_uses_official_lvgl_repository() -> None:
    source = read_repo_text(SETUP_SCRIPT)

    check(
        "https://github.com/lvgl/lv_port_pc_vscode.git" in source,
        "PC UI simulator setup must download the official LVGL lv_port_pc_vscode repository.",
    )
    check(
        "git@github.com:lvgl/lv_port_pc_vscode.git" in source,
        "PC UI simulator setup should support the official GitHub SSH URL when HTTPS is unavailable.",
    )
    check('"release/v8"' in source, "LVGL simulator should default to release/v8 to match firmware LVGL 8.x.")
    check("--branch $Branch" in source, "LVGL simulator setup must clone the selected official branch.")
    check("submodule update --init --recursive --depth 1" in source, "LVGL simulator setup must initialize submodules.")
    check("--depth 1" in source, "LVGL simulator clone and submodule update should stay lightweight.")
    check("Invoke-CheckedGit" in source, "Setup script must fail fast when git clone or submodule update fails.")
    check("submodule set-url lvgl git@github.com:lvgl/lvgl.git" in source, "SSH mode must switch the lvgl submodule to the official SSH URL.")
    check("submodule set-url lv_drivers git@github.com:lvgl/lv_drivers.git" in source, "SSH mode must switch the lv_drivers submodule to the official SSH URL.")


def test_simulator_checkout_is_not_committed_as_project_source() -> None:
    gitignore = read_repo_text(GITIGNORE)

    check(
        "tools/lv_port_pc_vscode/" in gitignore,
        "Downloaded LVGL simulator checkout must stay ignored by the BikeMB repo.",
    )


def test_simulator_open_script_builds_existing_checkout_only() -> None:
    source = read_repo_text(OPEN_SCRIPT)

    check(
        "tools\\setup-lvgl-simulator.ps1 first" in source,
        "Open script must ask for the official checkout instead of creating a custom simulator.",
    )
    check("make LV_DRIVER=$Driver" in source, "Open script must build the official simulator Makefile with a selectable LV_DRIVER.")
    check("C:\\msys64\\usr\\bin\\bash.exe" in source, "Open script should run the Linux-oriented official Makefile through MSYS2 bash on Windows.")
    check("Convert-ToMsysPath" in source, "Open script must convert the checkout path before invoking MSYS2 bash.")
    check("Start-Process" in source, "Open script must open the built PC simulator app for visual review.")
    check("CapturePath" in source, "Open script must accept a deterministic visual-QA capture path.")
    check("CapturePage" in source, "Open script must select the visual-QA dashboard page.")
    check(
        "BIKEMB_SIMULATOR_CAPTURE_PATH" in source,
        "Open script must pass the requested capture path to the simulator process.",
    )


def test_simulator_open_script_syncs_bikemb_ui_before_build() -> None:
    source = read_repo_text(OPEN_SCRIPT)

    check(
        "sync-bikemb-simulator-ui.ps1" in source,
        "Open script must sync BikeMB UI into the official simulator checkout before building.",
    )


def test_bikemb_simulator_ui_uses_official_ui_mechanism() -> None:
    sync_source = read_repo_text(SYNC_SCRIPT)
    ui_source = read_repo_text(BIKEMB_SIM_UI)
    header_source = read_repo_text(BIKEMB_SIM_UI_HEADER)

    check("tools\\simulator\\bikemb_ui" in sync_source, "Sync script must copy the tracked BikeMB UI source.")
    check("dashboard_view_core.c" in sync_source, "Sync script must copy the shared dashboard core into the simulator ui directory.")
    check("dashboard_view_core.h" in sync_source, "Sync script must copy the shared dashboard core header into the simulator ui directory.")
    check("Touch_CST816.h" in sync_source, "Sync script must provide a simulator touch-driver stub.")
    check("CST816_GESTURE_SWIPE_UP" in sync_source, "Simulator touch stub must include the new settings swipe gesture.")
    check("dashboard_pages.c" in sync_source, "Sync script must copy the shared dashboard pages source.")
    check("dashboard_ui_style.c" in sync_source, "Sync script must copy the shared dashboard style source.")
    check("SharedAssetsDir" in sync_source, "Sync script must copy generated dashboard visual assets.")
    check("dashboard_font_speed_140.c" in sync_source, "Sync script must include generated dashboard font assets.")
    check("dashboard_font_speed_decimal_96.c" in sync_source,
          "Sync script must include generated decimal dashboard font assets.")
    check("dashboard_img_home_bezel.c" in sync_source, "Sync script must include the independent bezel asset.")
    check("dashboard_font_output_80.c" in sync_source,
          "Sync script must include the generated output-value font.")
    check("lv_port_pc_vscode" in sync_source, "Sync script must target the official LVGL simulator checkout.")
    check("main\\src\\main.c" in sync_source, "Sync script must patch the official simulator main.c entry.")
    check("bikemb_dashboard_create();" in sync_source, "Sync script must switch simulator startup to BikeMB dashboard.")
    check("#define BIKEMB_SIMULATOR_UI 1" in sync_source, "Sync script must enable BikeMB UI by default.")

    check("void bikemb_dashboard_create(void)" in header_source, "BikeMB simulator UI must expose a C entry point.")
    check("bikemb_dashboard_create" in ui_source, "BikeMB simulator UI implementation must define the entry point.")
    check("lv_timer_create" in ui_source, "BikeMB simulator UI must update demo metrics without modifying simulator main loop.")
    check("BikeMbDashboardView_Create();" in ui_source, "BikeMB simulator UI must create the shared dashboard view.")
    check("BikeMbDashboardView_Update(&metrics);" in ui_source, "BikeMB simulator UI must update the shared dashboard view.")
    check(".ai =" in ui_source, "BikeMB simulator UI must populate the shared AI dashboard state.")
    check(
        "BIKEMB_SIMULATOR_CAPTURE_PATH" in ui_source,
        "Simulator must support a deterministic screenshot path for visual QA.",
    )
    check("SDL_RenderReadPixels" in ui_source, "Simulator visual QA must capture the rendered SDL framebuffer.")
    check("SDL_SaveBMP" in ui_source, "Simulator visual QA must save a reviewable image without external tools.")
    check("lv_timer_del" in ui_source, "Simulator screenshot capture must run only once.")
    check("capture_state ? 24.5f" in ui_source,
          "Visual QA capture must freeze the reference speed while normal simulation remains dynamic.")
    check("capture_state ? 45000000U" in ui_source,
          "Visual QA capture must freeze the reference time while normal simulation remains dynamic.")
    check("BIKEMB_SIMULATOR_CAPTURE_PAGE" in ui_source,
          "Visual QA capture must be able to select each dashboard page deterministically.")


def test_dashboard_view_core_is_shared_by_firmware_and_simulator() -> None:
    core_source = read_repo_text(SHARED_DASHBOARD_CORE)
    core_header = read_repo_text(SHARED_DASHBOARD_CORE_HEADER)
    pages_source = read_repo_text(SHARED_DASHBOARD_PAGES)
    firmware_wrapper = read_repo_text(FIRMWARE_DASHBOARD_WRAPPER)
    simulator_glue = read_repo_text(BIKEMB_SIM_UI)

    check("BikeMbDashboardMetrics" in core_header, "Shared dashboard core must expose a platform-neutral metrics struct.")
    check("BikeMbDashboardView_Create" in core_header, "Shared dashboard core must expose a create entry point.")
    check("BikeMbDashboardView_Update" in core_header, "Shared dashboard core must expose an update entry point.")
    check("BikeMbDashboardPages_Create" in core_source, "Shared dashboard core must delegate page creation to the page module.")
    check("ai_state_label" in pages_source, "Shared dashboard pages module must own AI page content rendering.")
    check("DISTANCE" in pages_source, "Shared dashboard pages module must own dashboard detail rendering.")

    check("dashboard_view_core.h" in firmware_wrapper, "Firmware dashboard wrapper must include the shared dashboard core.")
    check("BikeMbDashboardView_Create();" in firmware_wrapper, "Firmware dashboard create wrapper must delegate to shared core.")
    check("BikeMbDashboardView_Update(&viewMetrics);" in firmware_wrapper, "Firmware dashboard update wrapper must delegate to shared core.")
    check("lv_label_create" not in firmware_wrapper, "Firmware wrapper must not duplicate LVGL widget creation.")

    check("dashboard_view_core.h" in simulator_glue, "Simulator glue must include the shared dashboard core.")
    check("BikeMbDashboardView_Create();" in simulator_glue, "Simulator glue must delegate dashboard creation to shared core.")
    check("BikeMbDashboardView_Update(&metrics);" in simulator_glue, "Simulator glue must delegate dashboard updates to shared core.")
    check("BIKEMB LIVE" not in simulator_glue, "Simulator glue must not duplicate dashboard title rendering.")


def test_dashboard_content_layer_does_not_clip_text() -> None:
    core_source = read_repo_text(SHARED_DASHBOARD_CORE)
    pages_source = read_repo_text(SHARED_DASHBOARD_PAGES)
    style_source = read_repo_text(SHARED_DASHBOARD_STYLE)
    combined_ui_source = core_source + pages_source + style_source

    check(
        "lv_obj_set_style_clip_corner" not in combined_ui_source,
        "Dashboard content must not use rounded-corner clipping because it can cut labels on the round display edge.",
    )
    check(
        "LV_OPA_TRANSP" in style_source,
        "Dashboard should keep a transparent, non-clipping content layer above the round background.",
    )
    check(
        "BikeMbUi_MakeFixedLabel" in pages_source,
        "Dashboard stat labels should keep explicit widths to avoid overflow on the round display.",
    )


if __name__ == "__main__":
    test_simulator_setup_uses_official_lvgl_repository()
    test_simulator_checkout_is_not_committed_as_project_source()
    test_simulator_open_script_builds_existing_checkout_only()
    test_simulator_open_script_syncs_bikemb_ui_before_build()
    test_bikemb_simulator_ui_uses_official_ui_mechanism()
    test_dashboard_view_core_is_shared_by_firmware_and_simulator()
    test_dashboard_content_layer_does_not_clip_text()
    print("PASS test_lvgl_simulator_contract")
