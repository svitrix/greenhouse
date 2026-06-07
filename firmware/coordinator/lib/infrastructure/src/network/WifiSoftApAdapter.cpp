#include "WifiSoftApAdapter.hpp"
#include <WiFi.h>
#include <esp_mac.h>
#include <cstdio>

namespace gh::infra {

gh::domain::ErrorCode WifiSoftApAdapter::start(const char* ssid_prefix,
                                                const char* passphrase) noexcept {
    uint8_t mac[6] = {0};
    // eFuse-backed read works regardless of WiFi mode/init state; WiFi.macAddress()
    // returns zeros if called before the stack is up (→ "...-0000" SSID).
    esp_read_mac(mac, ESP_MAC_WIFI_STA);

    char ssid[64];
    std::snprintf(ssid, sizeof(ssid), "%s-%02X%02X",
                  ssid_prefix, mac[4], mac[5]);

    WiFi.mode(WIFI_AP);
    const char* pass = (passphrase != nullptr && passphrase[0] != '\0')
                       ? passphrase
                       : nullptr;
    if (!WiFi.softAP(ssid, pass)) {
        return gh::domain::ErrorCode::WifiAPModeFailed;
    }
    return gh::domain::ErrorCode::Ok;
}

void WifiSoftApAdapter::stop() noexcept {
    WiFi.softAPdisconnect(true);
}

IPAddress WifiSoftApAdapter::softApIP() const noexcept {
    return WiFi.softAPIP();
}

}
