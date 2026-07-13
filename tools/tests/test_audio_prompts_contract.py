from contract_helpers import REPO_ROOT, check, read_repo_text


MAIN = "firmware/bikemb/src/main.cpp"
CMAKE = "firmware/bikemb/src/CMakeLists.txt"
PLATFORMIO_INI = "firmware/bikemb/platformio.ini"
PROMPTS_HEADER = "firmware/bikemb/src/audio/audio_prompts.h"
PROMPTS_SOURCE = "firmware/bikemb/src/audio/audio_prompts.cpp"
PROMPT_ASSETS_HEADER = "firmware/bikemb/src/audio/audio_prompt_assets.h"
PROMPT_ASSETS_SOURCE = "firmware/bikemb/src/audio/audio_prompt_assets.cpp"
GENERATOR = "tools/generate-mode-prompts.ps1"
DASHBOARD_PAGES_HEADER = "firmware/bikemb/src/app/dashboard_pages.h"
DASHBOARD_PAGES_SOURCE = "firmware/bikemb/src/app/dashboard_pages.c"
DASHBOARD_CORE_HEADER = "firmware/bikemb/src/app/dashboard_view_core.h"
DASHBOARD_APP_HEADER = "firmware/bikemb/src/app/dashboard_app.h"


def test_mode_audio_prompts_are_present_but_default_off() -> None:
    main = read_repo_text(MAIN)

    check((REPO_ROOT / PROMPTS_HEADER).exists(), "Audio prompt header must exist.")
    check((REPO_ROOT / PROMPTS_SOURCE).exists(), "Audio prompt source must exist.")
    check((REPO_ROOT / PROMPT_ASSETS_HEADER).exists(), "Audio prompt asset header must exist.")
    check((REPO_ROOT / PROMPT_ASSETS_SOURCE).exists(), "Audio prompt asset source must exist.")

    header = read_repo_text(PROMPTS_HEADER)
    source = read_repo_text(PROMPTS_SOURCE)
    assets = read_repo_text(PROMPT_ASSETS_SOURCE)

    check("BIKE_MB_ENABLE_AUDIO_PROMPTS" in main, "main.cpp must define the audio prompt gate.")
    check("#define BIKE_MB_ENABLE_AUDIO_PROMPTS 0" in main, "Audio prompts must default to disabled.")
    check("BikeMbAudioPromptMode" in header, "Audio prompts must expose a mode enum.")
    check("BIKE_MB_AUDIO_PROMPT_MODE_ECO" in header, "Audio prompts must include ECO.")
    check("BIKE_MB_AUDIO_PROMPT_MODE_TRAIL" in header, "Audio prompts must include TRAIL.")
    check("BIKE_MB_AUDIO_PROMPT_MODE_BOOST" in header, "Audio prompts must include BOOST.")
    check("BikeMbAudioPrompts_Init" in header, "Audio prompts must expose init.")
    check("BikeMbAudioPrompts_PlayMode" in header, "Audio prompts must expose mode playback.")
    check("GPIO_NUM_15" in source, "Audio prompts must control the documented PA enable pin.")
    check("GPIO_NUM_48" in source and "GPIO_NUM_38" in source, "Audio prompts must use documented I2S clock pins.")
    check("GPIO_NUM_47" in source, "Audio prompts must use documented I2S speaker data pin.")
    check("kSampleRate = 16000" in source, "Audio prompts must play 16 kHz assets.")
    check("I2S_SLOT_MODE_STEREO" in source, "Audio prompts must output stereo I2S frames.")
    check("kBikeMbPromptEcoPcm" in assets, "Audio assets must include ECO prompt data.")
    check("kBikeMbPromptTrailPcm" in assets, "Audio assets must include TRAIL prompt data.")
    check("kBikeMbPromptBoostPcm" in assets, "Audio assets must include BOOST prompt data.")


def test_mode_audio_prompts_are_part_of_existing_bikemb_build() -> None:
    cmake = read_repo_text(CMAKE)
    main = read_repo_text(MAIN)
    config = read_repo_text(PLATFORMIO_INI)

    check("audio/audio_prompts.cpp" in cmake, "ESP-IDF CMake source list must include the prompt layer.")
    check("audio/audio_prompt_assets.cpp" in cmake, "ESP-IDF CMake source list must include prompt assets.")
    check('#include "audio/audio_prompts.h"' in main, "Main firmware must include audio prompts.")
    check("BikeMbAudioPrompts_Init();" in main, "Arduino setup must initialize audio prompts.")
    check("BikeMbAudioPrompts_PlayMode" in main, "Main firmware must route mode changes to prompt playback.")
    check(
        "[env:esp32-s3-touch-lcd-1-85c-mode-prompts-test]" in config,
        "Audio prompts must have an explicit opt-in PlatformIO environment.",
    )
    check(
        "-D BIKE_MB_ENABLE_AUDIO_PROMPTS=1" in config,
        "Audio prompt environment must enable BIKE_MB_ENABLE_AUDIO_PROMPTS.",
    )
    check(
        "default_envs = esp32-s3-touch-lcd-1-85c" in config,
        "Default PlatformIO env must remain the non-audio-prompt firmware.",
    )


def test_mode_changes_use_a_dashboard_callback() -> None:
    pages_header = read_repo_text(DASHBOARD_PAGES_HEADER)
    pages_source = read_repo_text(DASHBOARD_PAGES_SOURCE)
    core_header = read_repo_text(DASHBOARD_CORE_HEADER)
    app_header = read_repo_text(DASHBOARD_APP_HEADER)

    check("BikeMbDashboardModeChangedCallback" in core_header, "View core must define a mode-change callback type.")
    check("BikeMbDashboardPages_SetModeChangedCallback" in pages_header, "Pages must expose callback registration.")
    check("mode_changed_callback" in pages_source, "Pages must store the mode-change callback.")
    check("pages->mode_changed_callback(pages->home_mode_index)" in pages_source, "Mode click must emit new mode index.")
    check("BikeMbDashboardView_SetModeChangedCallback" in core_header, "View core must expose mode callback registration.")
    check("DashboardApp_SetModeChangedCallback" in app_header, "Dashboard app must expose mode callback registration.")


def test_local_tts_generator_documents_male_voice_and_phrases() -> None:
    check((REPO_ROOT / GENERATOR).exists(), "Local prompt generator script must exist.")
    script = read_repo_text(GENERATOR)

    check("Microsoft Kangkang" in script, "Generator must use the local zh-CN male voice.")
    check("经济模式" in script, "Generator must include the ECO phrase.")
    check("越野模式" in script, "Generator must include the TRAIL phrase.")
    check("增强模式" in script, "Generator must include the BOOST phrase.")
    check("audio_prompt_assets.cpp" in script, "Generator must write firmware prompt assets.")
    check("16000" in script, "Generator must require 16 kHz prompt WAV files.")
    check("16-bit mono PCM" in script, "Generator must document the expected WAV format.")


if __name__ == "__main__":
    test_mode_audio_prompts_are_present_but_default_off()
    test_mode_audio_prompts_are_part_of_existing_bikemb_build()
    test_mode_changes_use_a_dashboard_callback()
    test_local_tts_generator_documents_male_voice_and_phrases()
    print("PASS test_audio_prompts_contract")
