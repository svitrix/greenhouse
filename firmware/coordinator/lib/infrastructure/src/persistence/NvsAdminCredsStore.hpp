#pragma once

#include <Preferences.h>
#include "ports/IAdminCredsStore.hpp"

namespace gh::infra {

class NvsAdminCredsStore : public gh::domain::IAdminCredsStore {
public:
    [[nodiscard]] gh::domain::Result<gh::domain::AdminCreds> load()                                 noexcept override;
    [[nodiscard]] gh::domain::ErrorCode                      save(const gh::domain::AdminCreds& c) noexcept override;

private:
    // Namespace kept as "admin": the record migrates in place. Legacy records
    // simply lack the "iter" key, which getUInt() reports as its default (0 =
    // the self-describing legacy single-SHA marker), so an old record still
    // loads and verifies, then gets re-hashed with PBKDF2 + the "iter" key
    // written on next successful login.
    static constexpr const char* kNs      = "admin";
    static constexpr const char* kKeyUser = "user";
    static constexpr const char* kKeyHash = "pw_hash";
    static constexpr const char* kKeySalt = "salt";
    static constexpr const char* kKeyIter = "iter";
};

}  // namespace gh::infra
