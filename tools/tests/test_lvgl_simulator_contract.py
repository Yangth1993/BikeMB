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


if __name__ == "__main__":
    test_simulator_setup_uses_official_lvgl_repository()
    test_simulator_checkout_is_not_committed_as_project_source()
    test_simulator_open_script_builds_existing_checkout_only()
    print("PASS test_lvgl_simulator_contract")
