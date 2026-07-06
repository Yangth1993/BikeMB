from contract_helpers import check, read_repo_text


SETUP_SCRIPT = "tools/setup-lvgl-simulator.ps1"
OPEN_SCRIPT = "tools/open-lvgl-simulator.ps1"
GITIGNORE = ".gitignore"


def test_simulator_setup_uses_official_lvgl_repository() -> None:
    source = read_repo_text(SETUP_SCRIPT)

    check(
        "https://github.com/lvgl/lv_port_pc_vscode.git" in source,
        "PC UI simulator setup must download the official LVGL lv_port_pc_vscode repository.",
    )
    check("--recurse-submodules" in source, "LVGL simulator clone must initialize submodules.")
    check("--shallow-submodules" in source, "LVGL simulator clone should stay lightweight.")


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
    check("cmake -S . -B build" in source, "Open script must build the official simulator checkout with CMake.")
    check("Start-Process" in source, "Open script must open the built PC simulator app for visual review.")


if __name__ == "__main__":
    test_simulator_setup_uses_official_lvgl_repository()
    test_simulator_checkout_is_not_committed_as_project_source()
    test_simulator_open_script_builds_existing_checkout_only()
    print("PASS test_lvgl_simulator_contract")
