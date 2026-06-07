#pragma once
#include "entities/WifiCreds.hpp"
#include "errors/ErrorCode.hpp"
#include "util/Result.hpp"

namespace gh::domain {
struct IWifiCredsStore {
    virtual ~IWifiCredsStore() = default;
    [[nodiscard]] virtual Result<WifiCreds> load() noexcept = 0;
    [[nodiscard]] virtual ErrorCode save(WifiCreds creds) noexcept = 0;
};
}
