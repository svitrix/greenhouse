#pragma once

#include <Preferences.h>
#include "ports/ILastConnectErrorStore.hpp"

namespace gh::infra {

class NvsLastConnectErrorStore : public gh::domain::ILastConnectErrorStore {
public:
    [[nodiscard]] gh::domain::ConnectError load()                              noexcept override;
    [[nodiscard]] gh::domain::ErrorCode    save(gh::domain::ConnectError err) noexcept override;

private:
    static constexpr const char* kNs  = "wifi";
    static constexpr const char* kKey = "last_err";
};

}  // namespace gh::infra
