#pragma once

#include <cstdint>
#include "errors/ErrorCode.hpp"

namespace gh::domain {

// Last Wi-Fi STA disconnect reason, mapped from the ESP-IDF
// wifi_err_reason_t to a domain-level enum so the provisioning UI
// can render an actionable banner ("wrong password" vs "SSID not visible").
//
// Numeric values of wifi_err_reason_t constants vary across IDF versions.
// The mapping lives in WifiStaAdapter and uses constant names, not numbers.
enum class ConnectError : uint8_t {
    None         = 0,
    AuthFail     = 1,  // AUTH_FAIL / HANDSHAKE_TIMEOUT / 4WAY_HANDSHAKE_TIMEOUT
    SsidNotFound = 2,  // NO_AP_FOUND
    Timeout      = 3,  // BEACON_TIMEOUT
    Other        = 4,
};

class ILastConnectErrorStore {
public:
    virtual ~ILastConnectErrorStore() = default;

    [[nodiscard]] virtual ConnectError load()                 noexcept = 0;
    [[nodiscard]] virtual ErrorCode    save(ConnectError err) noexcept = 0;
};

}  // namespace gh::domain
