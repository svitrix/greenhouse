#pragma once

#include <cstdint>
#include "errors/ErrorCode.hpp"

namespace gh::domain {

// Persistent counter of consecutive failed Wi-Fi STA boot attempts.
// Incremented when WifiProvisioner exhausts its in-boot retry loop.
// Reset on a successful STA connect (WifiStaAdapter) or on entry to
// provisioning mode (WifiProvisioner).
class IWifiFailCounterStore {
public:
    virtual ~IWifiFailCounterStore() = default;

    [[nodiscard]] virtual uint8_t   load()      noexcept = 0;
    [[nodiscard]] virtual ErrorCode increment() noexcept = 0;
    [[nodiscard]] virtual ErrorCode reset()     noexcept = 0;
};

}  // namespace gh::domain
