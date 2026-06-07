#pragma once
#include <Preferences.h>
#include "ports/IMqttCredsStore.hpp"

namespace gh::infra {
class NvsMqttCredsStore final : public gh::domain::IMqttCredsStore {
public:
    NvsMqttCredsStore() noexcept = default;

    [[nodiscard]] gh::domain::ErrorCode begin() noexcept;
    [[nodiscard]] gh::domain::Result<gh::domain::MqttCreds>
        load() noexcept override;
    [[nodiscard]] gh::domain::ErrorCode
        save(gh::domain::MqttCreds creds) noexcept override;

private:
    Preferences prefs_;
    bool        opened_ = false;
};
}
