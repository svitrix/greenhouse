#pragma once
#include <atomic>
#include "ports/IWifiSta.hpp"
#include "ports/ILastConnectErrorStore.hpp"
#include "ports/IWifiFailCounterStore.hpp"
#include "ports/ILogger.hpp"

namespace gh::infra {
class WifiStaAdapter final : public gh::domain::IWifiSta {
public:
    WifiStaAdapter(gh::domain::ILastConnectErrorStore& lastError,
                   gh::domain::IWifiFailCounterStore&  failCounter,
                   gh::domain::ILogger&                log) noexcept;

    // One-shot setup — call from main.cpp before connect().
    // Registers WiFi.onEvent handler for ARDUINO_EVENT_WIFI_STA_DISCONNECTED
    // to populate ILastConnectErrorStore on disconnect.
    void begin() noexcept;

    [[nodiscard]] gh::domain::ErrorCode
        connect(const gh::domain::WifiCreds& creds,
                uint32_t timeout_ms) noexcept override;
    [[nodiscard]] bool isConnected() const noexcept override;

private:
    gh::domain::ILastConnectErrorStore& lastError_;
    gh::domain::IWifiFailCounterStore&  failCounter_;
    gh::domain::ILogger&                log_;
    // Set once the STA has associated at least once. Gates the auto-reconnect
    // in the disconnect handler so it only fires on *runtime* link drops, never
    // during the initial connect attempts (which the provisioner ladder owns).
    // Atomic: written from the connect() (setup) context, read from the WiFi
    // event-loop task in begin()'s disconnect handler.
    std::atomic<bool> connected_once_{false};
};
}
