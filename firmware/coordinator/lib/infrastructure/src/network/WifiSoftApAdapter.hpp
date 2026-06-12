#pragma once
#include "errors/ErrorCode.hpp"
#include <IPAddress.h>

namespace gh::infra {
class WifiSoftApAdapter {
public:
    WifiSoftApAdapter() noexcept = default;

    // passphrase must be 8..63 ASCII chars (WPA2-PSK requirement).
    // An empty/null passphrase is REJECTED with WifiAPModeFailed unless
    // allow_open is explicitly set true. Open AP carries the provisioning form
    // (Wi-Fi + MQTT passwords) in plaintext over the air, so it must be an
    // opt-in escape hatch, never a silent fallback.
    [[nodiscard]] gh::domain::ErrorCode start(const char* ssid_prefix,
                                              const char* passphrase,
                                              bool allow_open = false) noexcept;
    void stop() noexcept;

    [[nodiscard]] IPAddress softApIP() const noexcept;
};
}
