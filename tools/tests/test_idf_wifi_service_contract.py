from contract_helpers import check, read_repo_text


WIFI_SERVICE = "src/firmware/bikemb/src/network/wifi_service.cpp"
CMAKE = "src/firmware/bikemb/src/CMakeLists.txt"


def test_idf_wifi_service_uses_espidf_wifi_stack() -> None:
    source = read_repo_text(WIFI_SERVICE)

    check('"esp_wifi.h"' in source, "ESP-IDF Wi-Fi service must use esp_wifi.")
    check('"esp_netif.h"' in source, "ESP-IDF Wi-Fi service must initialize esp_netif.")
    check('"esp_event.h"' in source, "ESP-IDF Wi-Fi service must receive Wi-Fi/IP events.")
    check('"nvs_flash.h"' in source, "ESP-IDF Wi-Fi service must initialize NVS for Wi-Fi.")
    check("esp_wifi_connect()" in source, "ESP-IDF Wi-Fi service must start STA connection attempts.")
    check("IP_EVENT_STA_GOT_IP" in source, "ESP-IDF Wi-Fi service must mark connected from got-ip events.")
    check("WIFI_EVENT_STA_DISCONNECTED" in source, "ESP-IDF Wi-Fi service must mark disconnected from Wi-Fi events.")
    check("return s_idfConnected" in source, "ESP-IDF isConnected() must read real connection state.")


def test_idf_component_declares_wifi_dependencies() -> None:
    cmake = read_repo_text(CMAKE)

    for component in ("esp_wifi", "esp_netif", "esp_event", "nvs_flash"):
        check(component in cmake, f"ESP-IDF component must require {component}.")


if __name__ == "__main__":
    test_idf_wifi_service_uses_espidf_wifi_stack()
    test_idf_component_declares_wifi_dependencies()
    print("PASS test_idf_wifi_service_contract")
