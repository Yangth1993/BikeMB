from contract_helpers import check, find_function_body, read_repo_text


DISPLAY_DRIVER = "firmware/bikemb/src/drivers/Display_ST77916.cpp"
DISPLAY_DIAGNOSTICS = "firmware/bikemb/src/app/display_diagnostics.cpp"
MAIN = "firmware/bikemb/src/main.cpp"


def test_lcd_flush_transfer_callback_stays_disabled() -> None:
    source = read_repo_text(DISPLAY_DRIVER)

    check(
        ".on_color_trans_done = NULL" in source,
        "Panel IO color-transfer callback must stay disabled for the current synchronous LVGL flush contract.",
    )


def test_lcd_add_window_keeps_rgb565_byte_swap() -> None:
    source = read_repo_text(DISPLAY_DRIVER)
    body = find_function_body(source, "void LCD_addWindow(")
    pixel_body = find_function_body(source, "static inline uint16_t SwapRgb565Pixel(")
    swap_body = find_function_body(source, "static void SwapRgb565Buffer(")

    byte_swap_tokens = ["color[i]", ">> 8", "<< 8", "0xFF", "0xFF00"]
    check("color[i]" in swap_body, "SwapRgb565Buffer() must rewrite the RGB565 buffer in place.")
    for token in byte_swap_tokens[1:]:
        check(token in pixel_body, f"LCD_addWindow() must keep RGB565 byte-swap logic; missing {token}.")

    check(
        "SwapRgb565Buffer(color, size)" in body,
        "LCD_addWindow() must byte-swap the RGB565 buffer before drawing.",
    )
    check(
        "esp_lcd_panel_draw_bitmap" in body,
        "LCD_addWindow() must submit pixels with esp_lcd_panel_draw_bitmap().",
    )
    check(
        body.find("SwapRgb565Buffer(color, size)") < body.find("esp_lcd_panel_draw_bitmap"),
        "LCD_addWindow() must byte-swap the RGB565 buffer before esp_lcd_panel_draw_bitmap().",
    )


def test_lcd_add_window_collects_perf_stats_without_serial_logging() -> None:
    source = read_repo_text(DISPLAY_DRIVER)
    header = read_repo_text("firmware/bikemb/src/drivers/Display_ST77916.h")
    body = find_function_body(source, "void LCD_addWindow(")

    check("LCD_PerfStats" in header, "LCD driver must expose lightweight performance counters.")
    check("LCD_GetPerfStats" in header, "LCD driver must expose a stats snapshot getter.")
    check("LCD_ResetPerfStats" in header, "LCD driver must expose a stats reset helper.")
    check("BikePlatform_Micros()" in body, "LCD_addWindow() must measure synchronous LCD write time.")
    check("g_lcdPerfStats.flushCount" in body, "LCD_addWindow() must count flushes.")
    check("g_lcdPerfStats.pixelCount" in body, "LCD_addWindow() must count submitted pixels.")
    check("g_lcdPerfStats.totalWriteUs" in body, "LCD_addWindow() must accumulate write time.")
    check("Serial." not in body, "LCD_addWindow() must not print from the display hot path.")


def test_display_diagnostic_entry_stays_available_but_disabled_by_default() -> None:
    main_source = read_repo_text(MAIN)
    diagnostic_source = read_repo_text(DISPLAY_DIAGNOSTICS)

    check(
        "#define BIKE_MB_RUN_DISPLAY_DIAGNOSTIC 0" in main_source,
        "BIKE_MB_RUN_DISPLAY_DIAGNOSTIC must default to 0.",
    )
    check(
        "#if BIKE_MB_RUN_DISPLAY_DIAGNOSTIC" in main_source,
        "Main firmware must keep the diagnostic compile-time entry guard.",
    )
    check(
        "DisplayDiagnostics_Run();" in main_source,
        "Main firmware must keep the display diagnostic entry call.",
    )
    check(
        "void DisplayDiagnostics_Run()" in diagnostic_source,
        "DisplayDiagnostics_Run() implementation must remain available.",
    )
    check(
        "MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL" in diagnostic_source,
        "Display diagnostic buffer should stay DMA/internal-capable.",
    )


if __name__ == "__main__":
    test_lcd_flush_transfer_callback_stays_disabled()
    test_lcd_add_window_keeps_rgb565_byte_swap()
    test_lcd_add_window_collects_perf_stats_without_serial_logging()
    test_display_diagnostic_entry_stays_available_but_disabled_by_default()
    print("PASS test_display_driver_contract")
