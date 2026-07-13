import re

from contract_helpers import check, find_function_body, read_repo_text


LVGL_PORT = "firmware/bikemb/src/platform/lvgl_port.cpp"


def test_display_driver_has_static_lifetime() -> None:
    source = read_repo_text(LVGL_PORT)
    init_body = find_function_body(source, "void LvglPort_Init(")

    check(
        re.search(r"(?m)^\s*(?:static\s+)?lv_disp_drv_t\s+g_dispDrv\s*;", source) is not None,
        "lv_disp_drv_t g_dispDrv must remain at file/global scope; LVGL stores the registered driver pointer.",
    )
    check(
        "lv_disp_drv_init(&g_dispDrv)" in init_body,
        "LvglPort_Init() must initialize the file-scope g_dispDrv object.",
    )
    check(
        "lv_disp_drv_register(&g_dispDrv)" in init_body,
        "LvglPort_Init() must register the file-scope g_dispDrv object.",
    )

    local_driver = re.search(r"\blv_disp_drv_t\s+(?!g_dispDrv\b)[A-Za-z_]\w*\s*(?:[;=])", init_body)
    check(
        local_driver is None,
        "Do not declare a local lv_disp_drv_t inside LvglPort_Init(); it can cause LVGL use-after-return crashes.",
    )


def test_flush_callback_stays_synchronous_and_simple() -> None:
    source = read_repo_text(LVGL_PORT)
    flush_body = find_function_body(source, "void FlushCallback(")

    check("LCD_addWindow(" in flush_body, "FlushCallback() must draw through LCD_addWindow().")
    check(
        "lv_disp_flush_ready(disp)" in flush_body,
        "FlushCallback() must call lv_disp_flush_ready(disp) after LCD_addWindow().",
    )
    check(
        flush_body.find("LCD_addWindow(") < flush_body.find("lv_disp_flush_ready(disp)"),
        "FlushCallback() must mark the LVGL flush ready only after LCD_addWindow().",
    )

    forbidden_tokens = [
        "LCD_SetFlushDoneCallback",
        "WaitForLcdTransferDone",
        "LcdTransferDoneCallback",
        "on_color_trans_done",
        "while",
        "xSemaphore",
    ]
    for token in forbidden_tokens:
        check(
            token not in flush_body,
            f"FlushCallback() must not reintroduce async transfer/wait-loop logic: found {token}.",
        )


def test_lvgl_flush_metrics_are_collected_without_async_flush() -> None:
    source = read_repo_text(LVGL_PORT)
    header = read_repo_text("firmware/bikemb/src/platform/lvgl_port.h")
    flush_body = find_function_body(source, "void FlushCallback(")
    run_body = find_function_body(source, "uint32_t LvglPort_Run(")

    check("LvglPortPerfStats" in header, "LVGL port must expose lightweight performance counters.")
    check("LvglPort_GetPerfStats" in header, "LVGL port must expose a stats snapshot getter.")
    check("LvglPort_ResetPerfStats" in header, "LVGL port must expose a stats reset helper.")
    check("g_lvglPerfStats.flushCount" in flush_body, "FlushCallback() must count LVGL flushes.")
    check("g_lvglPerfStats.flushPixelCount" in flush_body, "FlushCallback() must count flushed pixels.")
    check("handlerTotalUs" in run_body, "LvglPort_Run() must accumulate lv_timer_handler() time.")
    check("handlerMaxUs" in run_body, "LvglPort_Run() must track max lv_timer_handler() time.")


def test_lvgl_draw_buffers_remain_internal_ram_and_use_40_lines() -> None:
    source = read_repo_text(LVGL_PORT)

    check(
        "kBufferLines = 40" in source,
        "LVGL draw buffer should use the measured 40-line profile for the stable 40 FPS pass.",
    )
    check(
        "MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT" in source,
        "LVGL draw buffers should stay in internal 8-bit capable RAM for this bring-up profile.",
    )
    check(
        "MALLOC_CAP_SPIRAM" not in source,
        "Do not move LVGL draw buffers to PSRAM without a dedicated memory/performance review.",
    )


if __name__ == "__main__":
    test_display_driver_has_static_lifetime()
    test_flush_callback_stays_synchronous_and_simple()
    test_lvgl_flush_metrics_are_collected_without_async_flush()
    test_lvgl_draw_buffers_remain_internal_ram_and_use_40_lines()
    print("PASS test_lvgl_port_contract")
