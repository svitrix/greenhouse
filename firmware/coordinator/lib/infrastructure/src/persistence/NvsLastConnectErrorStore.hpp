#pragma once

#include <Preferences.h>
#include "ports/ILastConnectErrorStore.hpp"

namespace gh::infra {

class NvsLastConnectErrorStore : public gh::domain::ILastConnectErrorStore {
public:
    [[nodiscard]] gh::domain::ConnectError load()                              noexcept override;
    [[nodiscard]] gh::domain::ErrorCode    save(gh::domain::ConnectError err) noexcept override;

private:
    // Own namespace per the persistence spec (was multiplexed under "wifi").
    // Migration: the old "wifi"/"last_err" value is intentionally NOT carried
    // over — this is a diagnostic-only field (shown in the SPA), so it simply
    // reads back as None until the next failed connect populates it.
    static constexpr const char* kNs  = "last_err";
    static constexpr const char* kKey = "code";
};

}  // namespace gh::infra
