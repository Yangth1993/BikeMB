set(BIKEMB_SRCS
    ai/ai_assistant.cpp
    ai/ai_state_machine.cpp
    ai/cloud_worker.cpp
    ai/cosyvoice_tts_adapter.cpp
    ai/deepseek_adapter.cpp
    ai/qwen_asr_adapter.cpp
    ai/qwen_chat_adapter.cpp
    app/ai_assistant_ui_state.cpp
    app/dashboard_app.cpp
    app/assets/dashboard_font_speed_140.c
    app/assets/dashboard_font_speed_decimal_96.c
    app/assets/dashboard_font_output_80.c
    app/assets/dashboard_img_home_assist_glow.c
    app/assets/dashboard_img_home_bezel.c
    app/dashboard_pages.c
    app/dashboard_ui_style.c
    app/dashboard_view.cpp
    app/dashboard_view_core.c
    audio/audio_capture_core.cpp
    audio/audio_capture_self_test.cpp
    audio/audio_prompt_assets.cpp
    audio/audio_prompts.cpp
    audio/audio_session.cpp
    audio/audio_session_core.cpp
    audio/audio_self_test.cpp
    app/demo_metrics.cpp
    app/display_diagnostics.cpp
    drivers/Display_ST77916.cpp
    drivers/I2C_Driver.cpp
    drivers/TCA9554PWR.cpp
    drivers/Touch_CST816.cpp
    drivers/esp_lcd_st77916.c
    input/ai_button.cpp
    input/ai_button_logic.cpp
    main.cpp
    network/wifi_service.cpp
    network/wifi_service_core.cpp
    platform/board_support.cpp
    platform/lvgl_port.cpp
    runtime/bike_runtime.cpp
    runtime/bike_runtime_plan.cpp
    services/metrics_service.cpp
    services/ui_service.cpp
    voice/voice_commands.cpp
)

idf_component_register(
    SRCS ${BIKEMB_SRCS}
    INCLUDE_DIRS "." "../include"
    REQUIRES driver esp_timer esp_lcd esp_hw_support esp_event esp_netif esp_wifi nvs_flash lvgl
)

target_compile_definitions(${COMPONENT_LIB} PRIVATE
    LV_CONF_INCLUDE_SIMPLE
    BIKE_MB_USE_ESPIDF_RUNTIME=1
)
