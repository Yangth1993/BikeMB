from pathlib import Path

from contract_helpers import REPO_ROOT


FIRMWARE_ROOT = REPO_ROOT / "src" / "firmware" / "bikemb"


def read_text(path):
    return Path(path).read_text(encoding="utf-8")


def test_wifi_service_is_ai_feature_gated_and_nonblocking():
    main = read_text(FIRMWARE_ROOT / "src" / "main.cpp")
    service = read_text(FIRMWARE_ROOT / "src" / "network" / "wifi_service.cpp")

    assert '#include "network/wifi_service.h"' in main
    assert "BikeMbWifiService_Init();" in main
    assert "BikeMbWifiService_Init();" in main[
        main.index("#if BIKE_MB_ENABLE_AI_ASSISTANT") :
        main.index("#endif", main.index("BikeMbAiButton_Init();"))
    ]
    assert "while (WiFi.status()" not in service
    assert "delay(" not in service
    assert "vTaskDelay" in service
    assert "BikeMbAiAssistant_SetWifiConnected" in service


def test_dashboard_does_not_own_wifi_service():
    dashboard = read_text(FIRMWARE_ROOT / "src" / "app" / "dashboard_app.cpp")
    pages = read_text(FIRMWARE_ROOT / "src" / "app" / "dashboard_pages.c")

    assert "WifiService" not in dashboard
    assert "wifi_service" not in dashboard
    assert "WifiService" not in pages
    assert "wifi_service" not in pages


if __name__ == "__main__":
    test_wifi_service_is_ai_feature_gated_and_nonblocking()
    test_dashboard_does_not_own_wifi_service()
    print("PASS test_wifi_service_contract")
