import configparser

from contract_helpers import check, read_repo_text


PLATFORMIO_INI = "src/firmware/bikemb/platformio.ini"
SDKCONFIG_DEFAULTS = "src/firmware/bikemb/sdkconfig.defaults"
ENV_NAME = "esp32-s3-touch-lcd-1-85c"
ENV_SECTION = f"env:{ENV_NAME}"
IDF_ENV_NAME = "esp32-s3-touch-lcd-1-85c-idf"
IDF_ENV_SECTION = f"env:{IDF_ENV_NAME}"


def load_config() -> configparser.ConfigParser:
    parser = configparser.ConfigParser()
    parser.optionxform = str
    parser.read_string(read_repo_text(PLATFORMIO_INI))
    return parser


def test_platformio_target_env_and_board_stay_fixed() -> None:
    config = load_config()

    check(config.has_section("platformio"), "platformio.ini must keep a [platformio] section.")
    check(
        config.get("platformio", "core_dir", fallback="") == "../../../.pio-home",
        "PlatformIO core_dir should stay inside the repo-local ignored .pio-home cache.",
    )
    check(
        config.get("platformio", "build_dir", fallback="") == "../../build/pio-bikemb",
        "PlatformIO build_dir should stay inside the repo-local ignored build directory.",
    )
    check(
        config.get("platformio", "libdeps_dir", fallback="") == "../../build/pio-libdeps",
        "PlatformIO libdeps_dir should stay inside the repo-local ignored build directory.",
    )
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
        "Arduino fallback env must remain available while the ESP-IDF migration is proven.",
    )
    for section in (ENV_SECTION, IDF_ENV_SECTION):
        check(
            config.get(section, "board_build.flash_size", fallback="") == "16MB",
            f"{section} must keep the Waveshare board flash size at 16MB.",
        )
        check(
            config.get(section, "board_build.f_cpu", fallback="") == "240000000L",
            f"{section} must keep the ESP32-S3 CPU frequency at 240MHz.",
        )


def test_platformio_espidf_env_is_available_without_replacing_fallback() -> None:
    config = load_config()

    check(config.has_section(IDF_ENV_SECTION), f"platformio.ini must add [{IDF_ENV_SECTION}].")
    check(
        config.get(IDF_ENV_SECTION, "board", fallback="") == "esp32-s3-devkitc1-n16r8",
        "ESP-IDF env must use the known working esp32-s3-devkitc1-n16r8 board profile.",
    )
    check(
        config.get(IDF_ENV_SECTION, "framework", fallback="") == "espidf",
        "ESP-IDF migration env must use framework = espidf.",
    )
    check(
        "../../build/pio-bikemb/esp32-s3-touch-lcd-1-85c-idf/sdkconfig"
        in config.get(IDF_ENV_SECTION, "board_build.cmake_extra_args", fallback=""),
        "ESP-IDF sdkconfig output should stay in the repo-local ignored build directory.",
    )
    check(
        config.get(ENV_SECTION, "framework", fallback="") == "arduino",
        "Arduino fallback env must not be converted in-place during the first ESP-IDF migration pass.",
    )


def test_platformio_serial_monitor_stays_on_com5() -> None:
    config = load_config()

    for section in (ENV_SECTION, IDF_ENV_SECTION):
        check(config.get(section, "upload_port", fallback="") == "COM5", f"{section} upload_port must remain COM5.")
        check(config.get(section, "monitor_port", fallback="") == "COM5", f"{section} monitor_port must remain COM5.")
        check(
            config.get(section, "monitor_speed", fallback="") == "115200",
            f"{section} monitor_speed must remain 115200.",
        )


def test_platformio_lvgl_include_flags_stay_present() -> None:
    config = load_config()
    for section in (ENV_SECTION, IDF_ENV_SECTION):
        build_flags = config.get(section, "build_flags", fallback="")
        lib_deps = config.get(section, "lib_deps", fallback="")

        check("-D LV_CONF_INCLUDE_SIMPLE" in build_flags, f"{section} must keep -D LV_CONF_INCLUDE_SIMPLE.")
        check("-I include" in build_flags, f"{section} must keep -I include.")
        check("lvgl/lvgl@^8.4.0" in lib_deps, f"{section} must keep lvgl/lvgl@^8.4.0.")


def test_espidf_sdkconfig_defaults_match_board_and_dashboard() -> None:
    defaults = read_repo_text(SDKCONFIG_DEFAULTS)

    required_tokens = [
        "CONFIG_ESPTOOLPY_FLASHSIZE_16MB=y",
        'CONFIG_ESPTOOLPY_FLASHSIZE="16MB"',
        "CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ_240=y",
        "CONFIG_FREERTOS_HZ=1000",
        "CONFIG_SPIRAM=y",
        "CONFIG_SPIRAM_MODE_OCT=y",
        "CONFIG_SPIRAM_SPEED_80M=y",
        "CONFIG_LV_FONT_MONTSERRAT_12=y",
        "CONFIG_LV_FONT_MONTSERRAT_18=y",
        "CONFIG_LV_FONT_MONTSERRAT_22=y",
        "CONFIG_LV_FONT_MONTSERRAT_28=y",
        "CONFIG_LV_FONT_MONTSERRAT_48=y",
    ]
    for token in required_tokens:
        check(token in defaults, f"sdkconfig.defaults must keep {token}.")


if __name__ == "__main__":
    test_platformio_target_env_and_board_stay_fixed()
    test_platformio_espidf_env_is_available_without_replacing_fallback()
    test_platformio_serial_monitor_stays_on_com5()
    test_platformio_lvgl_include_flags_stay_present()
    test_espidf_sdkconfig_defaults_match_board_and_dashboard()
    print("PASS test_project_config_contract")
