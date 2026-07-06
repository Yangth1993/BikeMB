import configparser

from contract_helpers import check, read_repo_text


PLATFORMIO_INI = "firmware/bikemb/platformio.ini"
ENV_NAME = "esp32-s3-touch-lcd-1-85c"
ENV_SECTION = f"env:{ENV_NAME}"


def load_config() -> configparser.ConfigParser:
    parser = configparser.ConfigParser()
    parser.optionxform = str
    parser.read_string(read_repo_text(PLATFORMIO_INI))
    return parser


def test_platformio_target_env_and_board_stay_fixed() -> None:
    config = load_config()

    check(config.has_section("platformio"), "platformio.ini must keep a [platformio] section.")
    check(config.has_section(ENV_SECTION), f"platformio.ini must keep [{ENV_SECTION}].")
    check(
        ENV_NAME in config.get("platformio", "default_envs", fallback=""),
        f"default_envs must include {ENV_NAME}.",
    )
    check(
        config.get(ENV_SECTION, "board", fallback="") == "esp32-s3-devkitc1-n16r8",
        "Board must remain esp32-s3-devkitc1-n16r8.",
    )
    check(
        config.get(ENV_SECTION, "framework", fallback="") == "arduino",
        "Framework must remain arduino for the current bring-up project.",
    )


def test_platformio_serial_monitor_stays_on_com5() -> None:
    config = load_config()

    check(config.get(ENV_SECTION, "upload_port", fallback="") == "COM5", "upload_port must remain COM5.")
    check(config.get(ENV_SECTION, "monitor_port", fallback="") == "COM5", "monitor_port must remain COM5.")
    check(
        config.get(ENV_SECTION, "monitor_speed", fallback="") == "115200",
        "monitor_speed must remain 115200.",
    )


def test_platformio_lvgl_include_flags_stay_present() -> None:
    config = load_config()
    build_flags = config.get(ENV_SECTION, "build_flags", fallback="")
    lib_deps = config.get(ENV_SECTION, "lib_deps", fallback="")

    check("-D LV_CONF_INCLUDE_SIMPLE" in build_flags, "build_flags must keep -D LV_CONF_INCLUDE_SIMPLE.")
    check("-I include" in build_flags, "build_flags must keep -I include.")
    check("lvgl/lvgl@^8.4.0" in lib_deps, "lib_deps must keep lvgl/lvgl@^8.4.0.")


if __name__ == "__main__":
    test_platformio_target_env_and_board_stay_fixed()
    test_platformio_serial_monitor_stays_on_com5()
    test_platformio_lvgl_include_flags_stay_present()
    print("PASS test_project_config_contract")
