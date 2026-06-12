#include "ArduinoSystemInfo.hpp"
#include <WiFi.h>
#include <esp_mac.h>
#include <esp_timer.h>
#include <cstdio>
#include <cstring>

namespace gh::infra {

ArduinoSystemInfo::ArduinoSystemInfo(const char* firmware_version) noexcept
    : fw_version_(firmware_version) {}

void ArduinoSystemInfo::snapshot(gh::domain::SystemInfo& out) noexcept {
    uint8_t mac[6] = {};
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    // device_id uses only the lower 24 bits of the MAC (the OUI bytes mac[0..2]
    // are constant for Espressif silicon and carry no per-unit entropy). This
    // means two boards could in theory collide on the same MQTT topic prefix.
    // Acceptable for a single-household deployment (a handful of coordinators);
    // if this ever scales to many units on one broker, widen this to the full
    // 48-bit MAC (%02x x6) and bump the topic schema.
    std::snprintf(out.device_id, sizeof(out.device_id),
                  "greenhouse_%02x%02x%02x", mac[3], mac[4], mac[5]);
    std::strncpy(out.firmware_version, fw_version_, sizeof(out.firmware_version) - 1);
    out.firmware_version[sizeof(out.firmware_version) - 1] = '\0';
    const auto ip = WiFi.localIP();
    std::snprintf(out.ip, sizeof(out.ip), "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
    out.uptime_s = static_cast<uint32_t>(esp_timer_get_time() / 1'000'000LL);
    out.wifi_rssi_dbm = static_cast<int16_t>(WiFi.RSSI());
}

}
