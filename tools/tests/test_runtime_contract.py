from pathlib import Path

from contract_helpers import REPO_ROOT, check, read_repo_text


MAIN = "src/firmware/bikemb/src/main.cpp"
RUNTIME_HEADER = "src/firmware/bikemb/src/runtime/bike_runtime.h"
RUNTIME_SOURCE = "src/firmware/bikemb/src/runtime/bike_runtime.cpp"
EVENT_HEADER = "src/firmware/bikemb/src/runtime/bike_event.h"
UI_SERVICE = "src/firmware/bikemb/src/services/ui_service.cpp"
METRICS_SERVICE = "src/firmware/bikemb/src/services/metrics_service.cpp"


def test_native_espidf_entrypoint_delegates_to_runtime() -> None:
    source = read_repo_text(MAIN)

    check('extern "C" void app_main()' in source, "ESP-IDF build must provide an app_main() entrypoint.")
    check("BIKE_MB_USE_ESPIDF_RUNTIME" in source, "main.cpp must keep ESP-IDF and Arduino fallback paths explicit.")
    check("BikeRuntime_Init();" in source, "app_main() must initialize the BikeMB runtime.")
    check("BikeRuntime_Start();" in source, "app_main() must start the BikeMB runtime.")
    check("void setup()" in source and "void loop()" in source, "Arduino setup()/loop() fallback must remain available.")


def test_runtime_event_bus_uses_fixed_freertos_queue() -> None:
    header = read_repo_text(RUNTIME_HEADER)
    source = read_repo_text(RUNTIME_SOURCE)
    event_header = read_repo_text(EVENT_HEADER)

    check("BikeRuntime_Init" in header, "Runtime public API must expose BikeRuntime_Init().")
    check("BikeRuntime_Start" in header, "Runtime public API must expose BikeRuntime_Start().")
    check("BikeRuntime_PostEvent" in header, "Runtime public API must expose BikeRuntime_PostEvent().")
    check("enum class BikeEventType" in event_header, "Runtime must define typed BikeEventType values.")
    for event_name in ("SystemTick", "DashboardTick", "RenderStatsUpdate", "DiagnosticRequest"):
        check(event_name in event_header, f"BikeEventType must include {event_name}.")
    check("xQueueCreate" in source, "Runtime event bus must use a fixed FreeRTOS queue.")
    check("xQueueSend" in source, "BikeRuntime_PostEvent() must post through the FreeRTOS queue.")
    check("g_droppedLowPriorityEvents" in source, "Runtime must count dropped low-priority events.")
    check("xTaskCreatePinnedToCore" in source, "Runtime must start tasks through ESP-IDF FreeRTOS.")


def test_runtime_logs_idf_acceptance_resource_diagnostics() -> None:
    source = read_repo_text(RUNTIME_SOURCE)

    check("BikeRuntime_LogServicePlan" in source, "Runtime must log service/core ownership for IDF acceptance.")
    check("heap_caps_get_free_size" in source, "Runtime diagnostics must log heap and PSRAM free memory.")
    check("heap_caps_get_largest_free_block" in source, "Runtime diagnostics must log largest heap blocks.")
    check("uxTaskGetStackHighWaterMark(nullptr)" in source, "Runtime diagnostics must log runtime stack high-water mark.")
    check("BikeRuntime_GetDroppedLowPriorityEvents()" in source, "Runtime diagnostics must include dropped UI/runtime events.")


def test_ui_service_is_lvgl_single_owner() -> None:
    source = read_repo_text(UI_SERVICE)

    check("xQueueReceive" in source, "UI service must consume BikeEvent messages from the runtime queue.")
    check("LvglPort_Init();" in source, "UI service must initialize LVGL.")
    check("DashboardApp_Init();" in source, "UI service must create the dashboard.")
    check("DashboardApp_Tick(" in source, "UI service must drive dashboard updates from events.")
    check("LvglPort_Run()" in source, "UI service must be the runtime owner of lv_timer_handler().")
    check("ESP_LOG" in source, "UI service must use ESP-IDF logging instead of Serial logging.")
    check("Serial." not in source, "UI service must not use Arduino Serial.")


def test_ui_service_logs_lvgl_acceptance_diagnostics() -> None:
    source = read_repo_text(UI_SERVICE)

    check("LvglPort_GetPerfStats()" in source, "UI diagnostics must read LVGL perf stats from the UI task.")
    check("LvglPort_ResetPerfStats()" in source, "UI diagnostics must reset LVGL perf stats after each window.")
    check("handlerMaxUs" in source, "UI diagnostics must include max lv_timer_handler time.")
    check("flushCount" in source, "UI diagnostics must include LVGL flush count.")
    check("heap_caps_get_free_size" in source, "UI diagnostics must include heap and PSRAM free memory.")
    check("uxTaskGetStackHighWaterMark(nullptr)" in source, "UI diagnostics must log UI task stack high-water mark.")


def test_metrics_service_exists_as_app_service_boundary() -> None:
    source = read_repo_text(METRICS_SERVICE)

    check("MetricsService_Init" in source, "Metrics service must expose an init boundary.")
    check("MetricsService_UpdateDashboard" in source, "Metrics service must own demo metrics updates.")
    check("DemoMetrics_Update" in source, "Metrics service should wrap existing demo metrics instead of rewriting UI data.")


def test_no_runtime_source_uses_arduino_header() -> None:
    runtime_root = REPO_ROOT / "src" / "firmware" / "bikemb" / "src" / "runtime"
    service_root = REPO_ROOT / "src" / "firmware" / "bikemb" / "src" / "services"
    sources = list(runtime_root.glob("*.cpp")) + list(runtime_root.glob("*.h")) + list(service_root.glob("*.cpp")) + list(service_root.glob("*.h"))

    check(sources, "Runtime/service source files must exist.")
    for source_path in sources:
        text = source_path.read_text(encoding="utf-8")
        relative = Path(source_path).relative_to(REPO_ROOT)
        check("<Arduino.h>" not in text, f"{relative} must not include Arduino.h.")


if __name__ == "__main__":
    test_native_espidf_entrypoint_delegates_to_runtime()
    test_runtime_event_bus_uses_fixed_freertos_queue()
    test_runtime_logs_idf_acceptance_resource_diagnostics()
    test_ui_service_is_lvgl_single_owner()
    test_ui_service_logs_lvgl_acceptance_diagnostics()
    test_metrics_service_exists_as_app_service_boundary()
    test_no_runtime_source_uses_arduino_header()
    print("PASS test_runtime_contract")
