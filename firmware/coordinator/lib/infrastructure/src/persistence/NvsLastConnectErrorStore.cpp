#include "NvsLastConnectErrorStore.hpp"

namespace gh::infra {

gh::domain::ConnectError NvsLastConnectErrorStore::load() noexcept {
    Preferences p;
    if (!p.begin(kNs, /*readOnly=*/true)) return gh::domain::ConnectError::None;
    const uint8_t raw = p.getUChar(kKey, 0);
    p.end();
    if (raw > static_cast<uint8_t>(gh::domain::ConnectError::Other)) {
        return gh::domain::ConnectError::Other;
    }
    return static_cast<gh::domain::ConnectError>(raw);
}

gh::domain::ErrorCode NvsLastConnectErrorStore::save(gh::domain::ConnectError err) noexcept {
    Preferences p;
    if (!p.begin(kNs, /*readOnly=*/false)) return gh::domain::ErrorCode::ConfigStoreFailed;
    p.putUChar(kKey, static_cast<uint8_t>(err));
    p.end();
    return gh::domain::ErrorCode::Ok;
}

}  // namespace gh::infra
