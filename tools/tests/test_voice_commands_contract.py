from contract_helpers import REPO_ROOT, check, read_repo_text


MAIN = "src/firmware/bikemb/src/main.cpp"
CMAKE = "src/firmware/bikemb/src/CMakeLists.txt"
VOICE_HEADER = "src/firmware/bikemb/src/voice/voice_commands.h"
VOICE_SOURCE = "src/firmware/bikemb/src/voice/voice_commands.cpp"
PLATFORMIO_INI = "src/firmware/bikemb/platformio.ini"
SRMODEL_UPLOAD_SCRIPT = "tools/pio_upload_srmodels.py"


def test_voice_commands_are_present_but_default_off() -> None:
    main = read_repo_text(MAIN)

    check((REPO_ROOT / VOICE_HEADER).exists(), "Voice commands header must exist in the BikeMB firmware.")
    check((REPO_ROOT / VOICE_SOURCE).exists(), "Voice commands source must exist in the BikeMB firmware.")

    header = read_repo_text(VOICE_HEADER)
    source = read_repo_text(VOICE_SOURCE)

    check("BIKE_MB_ENABLE_VOICE_COMMANDS" in main, "main.cpp must define the voice command gate.")
    check("#define BIKE_MB_ENABLE_VOICE_COMMANDS 0" in main, "Voice commands must default to disabled.")
    check("BikeMbVoiceCommands_Init" in header, "Voice API must expose init.")
    check("BikeMbVoiceCommands_ConsumeCommand" in header, "Voice API must expose a command queue.")
    check("BIKE_MB_VOICE_COMMAND_NEXT_PAGE" in header, "Voice command enum must include next page.")
    check("BIKE_MB_VOICE_COMMAND_PREVIOUS_PAGE" in header, "Voice command enum must include previous page.")
    check('#include "ESP_SR.h"' in source, "Voice commands must use Arduino ESP-SR.")
    check("SR_MODE_COMMAND" in source, "Voice commands must start ESP-SR in direct command mode.")
    check('"Next page"' in source, "Voice commands must include the English next-page phrase.")
    check('"Previous page"' in source, "Voice commands must include the English previous-page phrase.")
    check("SR_CHANNELS_STEREO" in source, "Voice commands must use the verified stereo ES7210 input path.")
    check('"MN"' in source, "Voice commands must describe the ES7210 stereo stream as microphone plus unused channel.")
    check(
        "BIKE_MB_ENABLE_VOICE_COMMANDS &&" in main and "Voice Commands still own I2S0" in main,
        "Voice commands must stay compile-time exclusive until they have an AudioSession migration test.",
    )


def test_voice_commands_are_part_of_existing_bikemb_build() -> None:
    cmake = read_repo_text(CMAKE)
    main = read_repo_text(MAIN)

    check("voice/voice_commands.cpp" in cmake, "ESP-IDF CMake source list must include the voice layer.")
    check('#include "voice/voice_commands.h"' in main, "Main firmware must include the voice command layer.")
    check("BikeMbVoiceCommands_Init();" in main, "Arduino setup must initialize voice commands through the BikeMB layer.")
    check("BikeMbVoiceCommands_ConsumeCommand()" in main, "Main loop must consume voice commands.")
    check("DashboardApp_NextPage();" in main, "Voice next-page commands must route through DashboardApp.")
    check("DashboardApp_PreviousPage();" in main, "Voice previous-page commands must route through DashboardApp.")


def test_voice_commands_have_explicit_opt_in_build_environment() -> None:
    config = read_repo_text(PLATFORMIO_INI)

    check(
        "[env:esp32-s3-touch-lcd-1-85c-voice-direct-test]" in config,
        "Voice commands must have an explicit opt-in PlatformIO environment.",
    )
    check(
        "-D BIKE_MB_ENABLE_VOICE_COMMANDS=1" in config,
        "Voice command environment must enable BIKE_MB_ENABLE_VOICE_COMMANDS.",
    )
    voice_env = config.split("[env:esp32-s3-touch-lcd-1-85c-voice-direct-test]", 1)[1]
    voice_env = voice_env.split("\n[env:", 1)[0]
    check(
        "BIKE_MB_ENABLE_AUDIO_SESSION=1" not in voice_env,
        "Voice direct environment must not enable AudioSession before voice is migrated.",
    )
    check(
        "BIKE_MB_ENABLE_AUDIO_SELF_TEST=1" not in voice_env,
        "Voice direct environment must not enable audio self-test.",
    )
    check(
        "BIKE_MB_ENABLE_AUDIO_PROMPTS=1" not in voice_env,
        "Voice direct environment must not enable audio prompts.",
    )
    check(
        "board_build.partitions = esp_sr_16.csv" in config,
        "Voice command environment must use an ESP-SR compatible partition table.",
    )
    check(
        "extra_scripts = pre:../../../tools/pio_upload_srmodels.py" in config,
        "Voice command environment must upload ESP-SR model data before firmware verification.",
    )
    upload_script = read_repo_text(SRMODEL_UPLOAD_SCRIPT)
    check("srmodels.bin" in upload_script, "ESP-SR upload script must reference the local model image.")
    check("FLASH_EXTRA_IMAGES" in upload_script, "ESP-SR upload script must append the model to PlatformIO flash images.")
    check("0xC10000" in upload_script, "ESP-SR upload script must upload the model image at the model partition offset.")
    check(
        "default_envs = esp32-s3-touch-lcd-1-85c" in config,
        "Default PlatformIO env must remain the non-voice firmware.",
    )


if __name__ == "__main__":
    test_voice_commands_are_present_but_default_off()
    test_voice_commands_are_part_of_existing_bikemb_build()
    test_voice_commands_have_explicit_opt_in_build_environment()
    print("PASS test_voice_commands_contract")
