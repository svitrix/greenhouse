#pragma once

#include <cstddef>
#include <cstdint>
#include "errors/ErrorCode.hpp"
#include "util/Result.hpp"

namespace gh::domain {

struct AdminCreds {
    static constexpr std::size_t kUsernameMax = 31;
    static constexpr std::size_t kHashLen     = 32;  // SHA-256 / PBKDF2-HMAC-SHA256 output
    static constexpr std::size_t kSaltLen     = 16;

    // iterations == 0 is the self-describing legacy marker: the record was
    // written by the pre-remediation firmware as a single SHA-256(salt ||
    // password) round. Any non-zero value means PBKDF2-HMAC-SHA256 with that
    // iteration count. Storing the count alongside salt+hash keeps the record
    // self-describing, so the verify path never has to guess the KDF and a
    // future iteration-count bump migrates transparently on next login.
    static constexpr uint32_t kLegacySingleSha = 0;

    char     username[kUsernameMax + 1];          // null-terminated
    uint8_t  password_hash[kHashLen];             // PBKDF2-HMAC-SHA256(password, salt, iterations)
    uint8_t  salt[kSaltLen];
    uint32_t iterations;                          // 0 = legacy single SHA-256 (see above)

    [[nodiscard]] bool valid() const noexcept {
        return username[0] != '\0';
    }

    [[nodiscard]] bool isLegacyFormat() const noexcept {
        return iterations == kLegacySingleSha;
    }
};

class IAdminCredsStore {
public:
    virtual ~IAdminCredsStore() = default;

    [[nodiscard]] virtual Result<AdminCreds> load()                  noexcept = 0;
    [[nodiscard]] virtual ErrorCode          save(const AdminCreds&) noexcept = 0;
};

}  // namespace gh::domain
