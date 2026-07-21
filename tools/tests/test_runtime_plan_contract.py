from pathlib import Path
import subprocess
import tempfile

from contract_helpers import REPO_ROOT, check, read_repo_text


FIRMWARE_ROOT = REPO_ROOT / "src" / "firmware" / "bikemb"
RUNTIME_PLAN_HEADER = "src/firmware/bikemb/src/runtime/bike_runtime_plan.h"
RUNTIME_PLAN_SOURCE = "src/firmware/bikemb/src/runtime/bike_runtime_plan.cpp"
PLATFORMIO = "src/firmware/bikemb/platformio.ini"
RUNTIME_PLAN_TEST = REPO_ROOT / "tools" / "tests" / "runtime_plan_test.cpp"


def compile_and_run(name, sources):
    with tempfile.TemporaryDirectory(prefix="bikemb-runtime-") as temp_dir:
        output = Path(temp_dir) / f"{name}.exe"
        command = ["g++", "-std=c++17", "-Wall", "-Wextra", "-Werror"]
        command.extend(str(path) for path in sources)
        command.extend(["-o", str(output)])
        subprocess.run(command, check=True, cwd=REPO_ROOT)
        subprocess.run([str(output)], check=True, cwd=REPO_ROOT)


def test_runtime_plan_is_host_testable_and_documents_core_ownership() -> None:
    header = read_repo_text(RUNTIME_PLAN_HEADER)
    source = read_repo_text(RUNTIME_PLAN_SOURCE)

    check("BIKE_RUNTIME_CORE_RUNTIME = 0" in header, "Runtime core must be CPU0.")
    check("BIKE_RUNTIME_CORE_UI = 1" in header, "UI core must be CPU1.")
    check("BikeRuntimeServicePlan" in header, "Runtime plan must expose service ownership records.")
    for service in (
        "bike_runtime",
        "bike_ui",
        "bikemb_ai",
        "bikemb_cloud",
        "bikemb_wifi",
        "ai_button_poll",
        "audio_session",
    ):
        check(service in source, f"Runtime plan must include {service}.")


def test_runtime_plan_native_contract() -> None:
    compile_and_run(
        "runtime_plan_test",
        [
            FIRMWARE_ROOT / "src" / "runtime" / "bike_runtime_plan.cpp",
            RUNTIME_PLAN_TEST,
        ],
    )


def test_product_workers_are_pinned_to_runtime_core() -> None:
    runtime = read_repo_text("src/firmware/bikemb/src/runtime/bike_runtime.cpp")
    ui = read_repo_text("src/firmware/bikemb/src/services/ui_service.cpp")
    assistant = read_repo_text("src/firmware/bikemb/src/ai/ai_assistant.cpp")
    cloud = read_repo_text("src/firmware/bikemb/src/ai/cloud_worker.cpp")
    wifi = read_repo_text("src/firmware/bikemb/src/network/wifi_service.cpp")

    check("BIKE_RUNTIME_CORE_RUNTIME" in runtime, "bike_runtime must use the runtime core constant.")
    check("BIKE_RUNTIME_CORE_UI" in ui, "bike_ui must use the UI core constant.")
    check("xTaskCreatePinnedToCore" in assistant, "AI assistant task must be pinned.")
    check("BIKE_RUNTIME_CORE_RUNTIME" in assistant, "AI assistant task must run on CPU0.")
    check("xTaskCreatePinnedToCore" in cloud, "Cloud worker task must be pinned.")
    check("BIKE_RUNTIME_CORE_RUNTIME" in cloud, "Cloud worker task must run on CPU0.")
    check("xTaskCreatePinnedToCore" in wifi, "Wi-Fi worker task must be pinned.")
    check("BIKE_RUNTIME_CORE_RUNTIME" in wifi, "Wi-Fi worker task must run on CPU0.")


def test_ai_button_routes_ui_changes_through_runtime_event_queue() -> None:
    event_header = read_repo_text("src/firmware/bikemb/src/runtime/bike_event.h")
    button = read_repo_text("src/firmware/bikemb/src/input/ai_button.cpp")
    ui = read_repo_text("src/firmware/bikemb/src/services/ui_service.cpp")

    check("ShowAiPage" in event_header, "Runtime event type must include AI page navigation.")
    check("BikeRuntime_PostEvent" in button, "AI button must post UI navigation through the runtime queue.")
    check("DashboardApp_ShowAiPage" not in button, "AI button must not touch DashboardApp directly.")
    check("DashboardApp_ShowAiPage" in ui, "UI service must own the actual AI page switch.")


def test_idf_acceptance_environment_starts_mock_ai_workers() -> None:
    platformio = read_repo_text(PLATFORMIO)
    idf_start = platformio.index("[env:esp32-s3-touch-lcd-1-85c-idf]")
    idf_env = platformio[idf_start:]

    check(
        "-D BIKE_MB_ENABLE_AI_ASSISTANT=1" in idf_env,
        "ESP-IDF acceptance env must start AI assistant, cloud, Wi-Fi, and AI button polling.",
    )
    check(
        "-D BIKE_MB_AI_USE_MOCK_PROVIDERS=1" in idf_env,
        "ESP-IDF acceptance env must use mock providers until real ESP-IDF cloud/audio adapters migrate.",
    )


if __name__ == "__main__":
    test_runtime_plan_is_host_testable_and_documents_core_ownership()
    test_runtime_plan_native_contract()
    test_product_workers_are_pinned_to_runtime_core()
    test_ai_button_routes_ui_changes_through_runtime_event_queue()
    test_idf_acceptance_environment_starts_mock_ai_workers()
    print("PASS test_runtime_plan_contract")
