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

    byte_swap_tokens = ["color[i]", ">> 8", "<< 8", "0xFF", "0xFF00"]
    for token in byte_swap_tokens:
        check(token in body, f"LCD_addWindow() must keep RGB565 byte-swap logic; missing {token}.")

    check(
        "esp_lcd_panel_draw_bitmap" in body,
        "LCD_addWindow() must submit pixels with esp_lcd_panel_draw_bitmap().",
    )
    check(
        body.find("color[i]") < body.find("esp_lcd_panel_draw_bitmap"),
        "LCD_addWindow() must byte-swap the RGB565 buffer before esp_lcd_panel_draw_bitmap().",
    )


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
    test_display_diagnostic_entry_stays_available_but_disabled_by_default()
    print("PASS test_display_driver_contract")
