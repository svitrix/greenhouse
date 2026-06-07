#pragma once

#include <cstddef>
#include <cstdint>
#include "errors/ErrorCode.hpp"
#include "util/Result.hpp"

namespace gh::domain {

struct AdminCreds {
    static constexpr std::size_t kUsernameMax = 31;
    static constexpr std::size_t kHashLen     = 32;  // SHA-256
    static constexpr std::size_t kSaltLen     = 16;

    char    username[kUsernameMax + 1];           // null-terminated
    uint8_t password_hash[kHashLen];              // SHA-256(salt || password)
    uint8_t salt[kSaltLen];

    [[nodiscard]] bool valid() const noexcept {
        return username[0] != '\0';
    }
};

class IAdminCredsStore {
public:
    virtual ~IAdminCredsStore() = default;

    [[nodiscard]] virtual Result<AdminCreds> load()                  noexcept = 0;
    [[nodiscard]] virtual ErrorCode          save(const AdminCreds&) noexcept = 0;
};

}  // namespace gh::domain
