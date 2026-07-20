from contract_helpers import REPO_ROOT, check, read_repo_text


MAIN = "src/firmware/bikemb/src/main.cpp"
CMAKE = "src/firmware/bikemb/src/CMakeLists.txt"
AUDIO_HEADER = "src/firmware/bikemb/src/audio/audio_self_test.h"
AUDIO_SOURCE = "src/firmware/bikemb/src/audio/audio_self_test.cpp"
PLATFORMIO_INI = "src/firmware/bikemb/platformio.ini"
DASHBOARD_CORE_HEADER = "src/firmware/bikemb/src/app/dashboard_view_core.h"
DASHBOARD_CORE_SOURCE = "src/firmware/bikemb/src/app/dashboard_view_core.c"
DASHBOARD_APP_HEADER = "src/firmware/bikemb/src/app/dashboard_app.h"
DASHBOARD_APP_SOURCE = "src/firmware/bikemb/src/app/dashboard_app.cpp"
DASHBOARD_VIEW_HEADER = "src/firmware/bikemb/src/app/dashboard_view.h"
DASHBOARD_VIEW_SOURCE = "src/firmware/bikemb/src/app/dashboard_view.cpp"


def test_audio_self_test_is_present_but_default_off() -> None:
    main = read_repo_text(MAIN)
    check((REPO_ROOT / AUDIO_HEADER).exists(), "Audio self-test header must exist in the BikeMB firmware.")
    check((REPO_ROOT / AUDIO_SOURCE).exists(), "Audio self-test source must exist in the BikeMB firmware.")
    audio_header = read_repo_text(AUDIO_HEADER)
    audio_source = read_repo_text(AUDIO_SOURCE)

    check("BIKE_MB_ENABLE_AUDIO_SELF_TEST" in main, "main.cpp must define the audio self-test gate.")
    check("#define BIKE_MB_ENABLE_AUDIO_SELF_TEST 0" in main, "Audio self-test must default to disabled.")
    check("BikeMbAudioSelfTest_Init" in audio_header, "Audio self-test API must expose init.")
    check("BikeMbAudioSelfTest_Tick" in audio_header, "Audio self-test API must expose tick.")
    check("BikeMbAudioSelfTest_PlayPageTone" in audio_header, "Audio self-test API must expose page feedback tone.")
    check("BikeMbAudioSelfTest_ConsumeCommand" in audio_header, "Audio self-test API must expose a simulated command queue.")
    check('#include "audio_session.h"' in audio_source, "Audio self-test must use AudioSession.")
    check("ESP_I2S.h" not in audio_source, "Audio self-test must not create its own I2S owner.")
    check("I2C_Driver.h" not in audio_source, "Audio self-test must not initialize codecs directly.")
    check("I2SClass" not in audio_source, "Audio self-test must not allocate I2S0 directly.")
    check("initSpeakerCodec" not in audio_source, "Audio self-test speaker init must live in AudioSession.")
    check("initMicrophoneCodec" not in audio_source, "Audio self-test mic init must live in AudioSession.")
    check("BikeMbAudioSession_Acquire(BIKE_MB_AUDIO_SESSION_OWNER_SELF_TEST" in audio_source, "Audio self-test must acquire the SELF_TEST owner.")
    check("BikeMbAudioSession_WriteStereoPcm" in audio_source, "Audio self-test tones must write through AudioSession.")
    check("BikeMbAudioSession_ReadMicBytes" in audio_source, "Audio self-test mic RMS must read through AudioSession.")
    check("BikeMbAudioSelfTest_MicTask" in audio_source, "Audio self-test mic RMS must run outside the UI loop.")
    check("xTaskCreatePinnedToCore" in audio_source, "Audio self-test must create a background mic task.")
    tick_body = audio_source.split("void BikeMbAudioSelfTest_Tick(uint32_t nowMs)", 1)[1]
    tick_body = tick_body.split("void BikeMbAudioSelfTest_PlayPageTone", 1)[0]
    check("reportMicLevel();" not in tick_body, "Audio self-test tick must not block the UI loop with mic reads.")
    check("kSampleRate = 16000" in audio_source, "Audio self-test microphone path must use the official 16 kHz sample rate.")
    check(
        "I2S_SLOT_MODE_STEREO" in read_repo_text("src/firmware/bikemb/src/audio/audio_session.cpp"),
        "AudioSession must keep the official stereo I2S slot mode.",
    )


def test_audio_layer_is_part_of_the_existing_bikemb_build() -> None:
    cmake = read_repo_text(CMAKE)
    main = read_repo_text(MAIN)

    check("audio/audio_self_test.cpp" in cmake, "ESP-IDF CMake source list must include the audio layer.")
    check('#include "audio/audio_self_test.h"' in main, "Main firmware must include the BikeMB audio self-test layer.")
    check("BikeMbAudioSelfTest_Init();" in main, "Arduino setup must initialize audio self-test through the BikeMB layer.")
    check("BikeMbAudioSelfTest_Tick(now);" in main, "Arduino loop must tick audio self-test through the BikeMB layer.")


def test_audio_self_test_has_explicit_opt_in_build_environment() -> None:
    config = read_repo_text(PLATFORMIO_INI)

    check(
        "[env:esp32-s3-touch-lcd-1-85c-audio-self-test]" in config,
        "Audio self-test must have an explicit opt-in PlatformIO environment.",
    )
    check(
        "-D BIKE_MB_ENABLE_AUDIO_SELF_TEST=1" in config,
        "Audio self-test environment must enable BIKE_MB_ENABLE_AUDIO_SELF_TEST.",
    )
    self_test_env = config.split("[env:esp32-s3-touch-lcd-1-85c-audio-self-test]", 1)[1]
    self_test_env = self_test_env.split("\n[env:", 1)[0]
    check(
        "-D BIKE_MB_ENABLE_AUDIO_SESSION=1" in self_test_env,
        "Migrated audio self-test environment must enable AudioSession.",
    )
    check(
        "default_envs = esp32-s3-touch-lcd-1-85c" in config,
        "Default PlatformIO env must remain the non-audio firmware.",
    )


def test_audio_self_test_has_speaker_only_diagnostic_environment() -> None:
    config = read_repo_text(PLATFORMIO_INI)
    audio_source = read_repo_text(AUDIO_SOURCE)

    check(
        "BIKE_MB_AUDIO_SELF_TEST_DISABLE_MIC" in audio_source,
        "Audio self-test must expose a compile-time mic-read diagnostic gate.",
    )
    check(
        "[env:esp32-s3-touch-lcd-1-85c-audio-self-test-speaker-only]" in config,
        "Audio self-test must have a speaker-only diagnostic PlatformIO environment.",
    )
    speaker_only_env = config.split("[env:esp32-s3-touch-lcd-1-85c-audio-self-test-speaker-only]", 1)[1]
    speaker_only_env = speaker_only_env.split("\n[env:", 1)[0]
    check(
        "-D BIKE_MB_ENABLE_AUDIO_SESSION=1" in speaker_only_env,
        "Speaker-only diagnostic env must still enable AudioSession.",
    )
    check(
        "-D BIKE_MB_ENABLE_AUDIO_SELF_TEST=1" in speaker_only_env,
        "Speaker-only diagnostic env must still enable audio self-test tone playback.",
    )
    check(
        "-D BIKE_MB_AUDIO_SELF_TEST_DISABLE_MIC=1" in speaker_only_env,
        "Speaker-only diagnostic env must disable mic reads.",
    )


def test_dashboard_page_commands_are_public_and_touch_uses_them() -> None:
    header = read_repo_text(DASHBOARD_CORE_HEADER)
    source = read_repo_text(DASHBOARD_CORE_SOURCE)
    app_header = read_repo_text(DASHBOARD_APP_HEADER)
    app_source = read_repo_text(DASHBOARD_APP_SOURCE)
    view_header = read_repo_text(DASHBOARD_VIEW_HEADER)
    view_source = read_repo_text(DASHBOARD_VIEW_SOURCE)
    main = read_repo_text(MAIN)

    check("BikeMbDashboardView_NextPage" in header, "Dashboard core must expose a next-page command.")
    check("BikeMbDashboardView_PreviousPage" in header, "Dashboard core must expose a previous-page command.")
    check("BikeMbDashboardView_NextPage();" in source, "Swipe-left handling must use the shared next-page command.")
    check("BikeMbDashboardView_PreviousPage();" in source, "Swipe-right handling must use the shared previous-page command.")
    check("DashboardApp_NextPage" in app_header, "Dashboard app must expose a next-page command.")
    check("DashboardApp_PreviousPage" in app_header, "Dashboard app must expose a previous-page command.")
    check("DashboardView_NextPage();" in app_source, "Dashboard app next-page command must delegate to the view layer.")
    check("DashboardView_PreviousPage();" in app_source, "Dashboard app previous-page command must delegate to the view layer.")
    check("DashboardView_NextPage" in view_header, "Dashboard view wrapper must expose a next-page command.")
    check("DashboardView_PreviousPage" in view_header, "Dashboard view wrapper must expose a previous-page command.")
    check("BikeMbDashboardView_NextPage();" in view_source, "Dashboard view wrapper must delegate next-page command to shared core.")
    check("BikeMbDashboardView_PreviousPage();" in view_source, "Dashboard view wrapper must delegate previous-page command to shared core.")
    check("BikeMbAudioSelfTest_ConsumeCommand()" in main, "Main loop must consume simulated audio commands.")
    check("DashboardApp_NextPage();" in main, "Main loop must route simulated next-page commands through DashboardApp.")
    check("DashboardApp_PreviousPage();" in main, "Main loop must route simulated previous-page commands through DashboardApp.")


if __name__ == "__main__":
    test_audio_self_test_is_present_but_default_off()
    test_audio_layer_is_part_of_the_existing_bikemb_build()
    test_audio_self_test_has_explicit_opt_in_build_environment()
    test_audio_self_test_has_speaker_only_diagnostic_environment()
    test_dashboard_page_commands_are_public_and_touch_uses_them()
    print("PASS test_audio_self_test_contract")
