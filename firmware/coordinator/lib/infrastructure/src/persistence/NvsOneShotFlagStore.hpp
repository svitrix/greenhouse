#pragma once
#include "ports/IOneShotFlagStore.hpp"
#include <Preferences.h>

namespace gh::infra {

class NvsOneShotFlagStore final : public gh::domain::IOneShotFlagStore {
public:
    [[nodiscard]] bool isSet(const char* key) noexcept override;
    [[nodiscard]] bool set(const char* key)   noexcept override;
private:
    static constexpr const char* kNs = "nvs_flags";
};

}
