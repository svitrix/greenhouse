#pragma once
#include <cstring>
#include "ports/IAdminCredsStore.hpp"

namespace gh::test {

class FakeAdminCredsStore : public gh::domain::IAdminCredsStore {
public:
    gh::domain::AdminCreds stored{};
    bool                   has_value  = false;
    int                    save_calls = 0;

    [[nodiscard]] gh::domain::Result<gh::domain::AdminCreds> load() noexcept override {
        if (!has_value) {
            return { gh::domain::ErrorCode::ConfigNotFound, {} };
        }
        return { gh::domain::ErrorCode::Ok, stored };
    }

    [[nodiscard]] gh::domain::ErrorCode save(const gh::domain::AdminCreds& c) noexcept override {
        std::memcpy(&stored, &c, sizeof(c));
        has_value = true;
        ++save_calls;
        return gh::domain::ErrorCode::Ok;
    }

    // Test helper: seed with a known user + already-hashed creds.
    void seed(const char* user, const uint8_t hash[32], const uint8_t salt[16]) {
        std::strncpy(stored.username, user, gh::domain::AdminCreds::kUsernameMax);
        stored.username[gh::domain::AdminCreds::kUsernameMax] = '\0';
        std::memcpy(stored.password_hash, hash, 32);
        std::memcpy(stored.salt,          salt, 16);
        has_value = true;
    }
};

}  // namespace gh::test
