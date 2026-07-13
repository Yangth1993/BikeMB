from contract_helpers import check, read_repo_text


MAIN = "firmware/bikemb/src/main.cpp"
DASHBOARD_APP = "firmware/bikemb/src/app/dashboard_app.cpp"
DASHBOARD_VIEW_CORE = "firmware/bikemb/src/app/dashboard_view_core.c"
DEMO_METRICS = "firmware/bikemb/src/app/demo_metrics.cpp"
LVGL_PORT = "firmware/bikemb/src/platform/lvgl_port.cpp"


def test_dashboard_uses_stable_cadence_for_synchronous_flush() -> None:
    app_source = read_repo_text(DASHBOARD_APP)
    main_source = read_repo_text(MAIN)

    check(
        "kFrameIntervalMs = 33" in app_source,
        "Dashboard app should stay on the proven 33ms cadence while LCD flush remains synchronous.",
    )
    check(
        "delay(5)" in main_source,
        "Main loop should keep the proven cooperative delay while LCD flush remains synchronous.",
    )
    check(
        "delay(1)" not in main_source,
        "Main loop must not use the aggressive 1ms delay again without async flush or board evidence.",
    )


def test_cpu_metric_uses_render_work_not_frame_interval() -> None:
    metrics_source = read_repo_text(DEMO_METRICS)
    app_source = read_repo_text(DASHBOARD_APP)
    port_source = read_repo_text(LVGL_PORT)

    check(
        "renderWorkMs" in metrics_source,
        "Demo CPU metric should be based on measured LVGL render work, not elapsed frame spacing.",
    )
    check(
        "elapsedMs * 100.0f" not in metrics_source,
        "Demo CPU metric must not treat normal frame interval timing as CPU load.",
    )
    check(
        "DashboardApp_SetRenderWorkMs" in app_source,
        "Dashboard app should accept the latest measured LVGL render work from the main loop.",
    )
    check(
        "BikePlatform_Micros()" in port_source,
        "LVGL port should measure lv_timer_handler() work duration through the platform timing wrapper.",
    )


def test_dashboard_view_avoids_unconditional_text_and_bar_redraws() -> None:
    core_source = read_repo_text(DASHBOARD_VIEW_CORE)

    check(
        "set_label_text_if_changed" in core_source,
        "Dashboard labels should only be updated when text changes to reduce redraw work.",
    )
    check(
        "set_bar_value_if_changed" in core_source,
        "Dashboard bars should only be updated when values change to reduce redraw work.",
    )
    check(
        "lv_label_get_text" in core_source,
        "Dashboard label update guard should compare against the existing LVGL label text.",
    )
    check(
        "lv_bar_get_value" in core_source,
        "Dashboard bar update guard should compare against the existing LVGL bar value.",
    )


def test_dashboard_hot_path_does_not_print_periodic_serial_logs() -> None:
    app_source = read_repo_text(DASHBOARD_APP)

    check(
        "Serial.printf" not in app_source,
        "Dashboard hot path must not print periodic serial logs because USB serial backpressure can stall FPS.",
    )
    check(
        "lastLogMs" not in app_source,
        "Dashboard hot path must not keep a periodic serial log timer.",
    )


if __name__ == "__main__":
    test_dashboard_uses_stable_cadence_for_synchronous_flush()
    test_cpu_metric_uses_render_work_not_frame_interval()
    test_dashboard_view_avoids_unconditional_text_and_bar_redraws()
    test_dashboard_hot_path_does_not_print_periodic_serial_logs()
    print("PASS test_dashboard_performance_contract")
