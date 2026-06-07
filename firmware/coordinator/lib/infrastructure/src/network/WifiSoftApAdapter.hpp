#pragma once
#include "errors/ErrorCode.hpp"
#include <IPAddress.h>

namespace gh::infra {
class WifiSoftApAdapter {
public:
    WifiSoftApAdapter() noexcept = default;

    // passphrase must be 8..63 ASCII chars (WPA2-PSK requirement).
    // Pass nullptr or empty string to fall back to an open AP (NOT recommended
    // for production — Wi-Fi password is then plaintext over the air).
    [[nodiscard]] gh::domain::ErrorCode start(const char* ssid_prefix,
                                              const char* passphrase) noexcept;
    void stop() noexcept;

    [[nodiscard]] IPAddress softApIP() const noexcept;
};
}
