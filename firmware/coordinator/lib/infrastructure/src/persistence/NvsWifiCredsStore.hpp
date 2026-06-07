#pragma once
#include <Preferences.h>
#include "ports/IWifiCredsStore.hpp"

namespace gh::infra {
class NvsWifiCredsStore final : public gh::domain::IWifiCredsStore {
public:
    NvsWifiCredsStore() noexcept = default;

    [[nodiscard]] gh::domain::ErrorCode begin() noexcept;
    [[nodiscard]] gh::domain::Result<gh::domain::WifiCreds>
        load() noexcept override;
    [[nodiscard]] gh::domain::ErrorCode
        save(gh::domain::WifiCreds creds) noexcept override;

private:
    Preferences prefs_;
    bool        opened_ = false;
};
}
