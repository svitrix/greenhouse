#pragma once

#include <Preferences.h>
#include "ports/IWifiFailCounterStore.hpp"

namespace gh::infra {

class NvsWifiFailCounterStore : public gh::domain::IWifiFailCounterStore {
public:
    [[nodiscard]] uint8_t              load()      noexcept override;
    [[nodiscard]] gh::domain::ErrorCode increment() noexcept override;
    [[nodiscard]] gh::domain::ErrorCode reset()    noexcept override;

private:
    static constexpr const char* kNs  = "wifi";
    static constexpr const char* kKey = "fail_count";
};

}  // namespace gh::infra
