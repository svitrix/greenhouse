#include "NvsAdminCredsStore.hpp"
#include <cstring>

namespace gh::infra {

gh::domain::Result<gh::domain::AdminCreds> NvsAdminCredsStore::load() noexcept {
    Preferences p;
    if (!p.begin(kNs, /*readOnly=*/true)) {
        return { gh::domain::ErrorCode::ConfigStoreFailed, {} };
    }

    gh::domain::AdminCreds c{};
    const size_t n_user = p.getString(kKeyUser, c.username, sizeof(c.username));
    const size_t n_hash = p.getBytes(kKeyHash, c.password_hash, gh::domain::AdminCreds::kHashLen);
    const size_t n_salt = p.getBytes(kKeySalt, c.salt,          gh::domain::AdminCreds::kSaltLen);
    p.end();

    if (n_user == 0 || n_hash != gh::domain::AdminCreds::kHashLen
                    || n_salt != gh::domain::AdminCreds::kSaltLen
                    || !c.valid()) {
        return { gh::domain::ErrorCode::ConfigNotFound, {} };
    }
    return { gh::domain::ErrorCode::Ok, c };
}

gh::domain::ErrorCode NvsAdminCredsStore::save(const gh::domain::AdminCreds& c) noexcept {
    if (!c.valid()) return gh::domain::ErrorCode::ConfigStoreFailed;
    Preferences p;
    if (!p.begin(kNs, /*readOnly=*/false)) return gh::domain::ErrorCode::ConfigStoreFailed;

    const size_t nu = p.putString(kKeyUser, c.username);
    const size_t nh = p.putBytes(kKeyHash, c.password_hash, gh::domain::AdminCreds::kHashLen);
    const size_t ns = p.putBytes(kKeySalt, c.salt,          gh::domain::AdminCreds::kSaltLen);
    p.end();

    if (nu == 0 || nh != gh::domain::AdminCreds::kHashLen
                 || ns != gh::domain::AdminCreds::kSaltLen) {
        return gh::domain::ErrorCode::ConfigStoreFailed;
    }
    return gh::domain::ErrorCode::Ok;
}

}  // namespace gh::infra
