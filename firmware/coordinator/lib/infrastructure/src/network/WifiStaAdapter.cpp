#include "WifiStaAdapter.hpp"
#include <WiFi.h>
#include <Arduino.h>
#include <cstdio>

namespace gh::infra {

WifiStaAdapter::WifiStaAdapter(gh::domain::ILastConnectErrorStore& lastError,
                               gh::domain::IWifiFailCounterStore&  failCounter,
                               gh::domain::ILogger&                log) noexcept
    : lastError_(lastError), failCounter_(failCounter), log_(log) {}

void WifiStaAdapter::begin() noexcept {
    // Recover from runtime link drops (e.g. AP deauth / "CCMP replay detected"):
    // the STA only connects once at boot, so without this a dropped link would
    // leave the board running but unreachable until a reboot.
    WiFi.setAutoReconnect(true);

    // Registered once at boot — the WiFi event loop owns the lambda for the
    // rest of the device lifetime. Captures `this` so the handler can persist
    // the mapped ConnectError via ILastConnectErrorStore. Must be called
    // AFTER the WiFi subsystem is up (i.e. not from a file-scope static ctor).
    WiFi.onEvent([this](WiFiEvent_t /*event*/, WiFiEventInfo_t info) {
        using gh::domain::ConnectError;
        ConnectError ce = ConnectError::Other;
        switch (info.wifi_sta_disconnected.reason) {
            case WIFI_REASON_AUTH_FAIL:
            case WIFI_REASON_HANDSHAKE_TIMEOUT:
            case WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT:
                ce = ConnectError::AuthFail;
                break;
            case WIFI_REASON_NO_AP_FOUND:
                ce = ConnectError::SsidNotFound;
                break;
            case WIFI_REASON_BEACON_TIMEOUT:
                ce = ConnectError::Timeout;
                break;
            default:
                ce = ConnectError::Other;
                break;
        }
        (void) lastError_.save(ce);
        // Runtime link recovery: setAutoReconnect handles most cases, but some
        // disconnect reasons (assoc-leave, beacon-timeout, "CCMP replay") need
        // an explicit nudge. Only after the first successful association, and
        // never on wrong-password / SSID-gone (retrying there just spams).
        if (connected_once_ &&
            ce != ConnectError::AuthFail &&
            ce != ConnectError::SsidNotFound) {
            WiFi.reconnect();
        }
    }, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
}

gh::domain::ErrorCode
WifiStaAdapter::connect(const gh::domain::WifiCreds& creds,
                        uint32_t timeout_ms) noexcept {
    if (!creds.valid()) return gh::domain::ErrorCode::WifiNotProvisioned;
    WiFi.mode(WIFI_STA);
    // Disable Wi-Fi modem power-save (WIFI_PS_NONE). The coex guide prefers
    // default PS, but our link instability is a Wi-Fi<->AP "CCMP replay" issue
    // (fires at association, before Zigbee starts), not coexistence — and an
    // A/B on hardware showed NONE reachable ~23% vs default ~0% in-window.
    // Keeping the radio always-on gives the most robust link with this AP.
    WiFi.setSleep(false);
    if (creds.hostname[0] != '\0') {
        WiFi.setHostname(creds.hostname);
    }
    WiFi.begin(creds.ssid, creds.password);
    const uint32_t start = millis();
    while ((millis() - start) < timeout_ms) {
        if (WiFi.status() == WL_CONNECTED) {
            (void) lastError_.save(gh::domain::ConnectError::None);
            (void) failCounter_.reset();
            connected_once_ = true;  // arm runtime auto-reconnect (see begin())
            char buf[48];
            std::snprintf(buf, sizeof(buf), "connected rssi=%d dBm ch=%d",
                          static_cast<int>(WiFi.RSSI()),
                          static_cast<int>(WiFi.channel()));
            log_.info("wifi", buf);
            return gh::domain::ErrorCode::Ok;
        }
        delay(100);
    }
    // Soft disconnect — keep the radio powered so the caller can retry
    // without re-issuing WiFi.mode(WIFI_STA). The `true` argument means
    // `wifioff` and would shut the radio down entirely.
    WiFi.disconnect(false);
    return gh::domain::ErrorCode::WifiConnectFailed;
}

bool WifiStaAdapter::isConnected() const noexcept {
    return WiFi.status() == WL_CONNECTED;
}

}  // namespace gh::infra
