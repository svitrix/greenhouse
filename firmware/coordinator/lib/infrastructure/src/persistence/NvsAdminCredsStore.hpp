#pragma once

#include <Preferences.h>
#include "ports/IAdminCredsStore.hpp"

namespace gh::infra {

class NvsAdminCredsStore : public gh::domain::IAdminCredsStore {
public:
    [[nodiscard]] gh::domain::Result<gh::domain::AdminCreds> load()                                 noexcept override;
    [[nodiscard]] gh::domain::ErrorCode                      save(const gh::domain::AdminCreds& c) noexcept override;

private:
    static constexpr const char* kNs      = "admin";
    static constexpr const char* kKeyUser = "user";
    static constexpr const char* kKeyHash = "pw_hash";
    static constexpr const char* kKeySalt = "salt";
};

}  // namespace gh::infra
