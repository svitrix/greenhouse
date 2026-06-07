#pragma once
#include "entities/WifiCreds.hpp"
#include "errors/ErrorCode.hpp"

namespace gh::domain {
struct IWifiSta {
    virtual ~IWifiSta() = default;
    [[nodiscard]] virtual ErrorCode connect(const WifiCreds& creds,
                                             uint32_t timeout_ms) noexcept = 0;
    [[nodiscard]] virtual bool isConnected() const noexcept = 0;
};
}
