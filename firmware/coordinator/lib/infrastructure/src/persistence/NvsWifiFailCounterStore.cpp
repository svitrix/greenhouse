#include "NvsWifiFailCounterStore.hpp"

namespace gh::infra {

uint8_t NvsWifiFailCounterStore::load() noexcept {
    Preferences p;
    if (!p.begin(kNs, /*readOnly=*/true)) return 0;
    const uint8_t v = p.getUChar(kKey, 0);
    p.end();
    return v;
}

gh::domain::ErrorCode NvsWifiFailCounterStore::increment() noexcept {
    Preferences p;
    if (!p.begin(kNs, /*readOnly=*/false)) return gh::domain::ErrorCode::ConfigStoreFailed;
    const uint8_t v = p.getUChar(kKey, 0);
    p.putUChar(kKey, static_cast<uint8_t>(v + 1));
    p.end();
    return gh::domain::ErrorCode::Ok;
}

gh::domain::ErrorCode NvsWifiFailCounterStore::reset() noexcept {
    Preferences p;
    if (!p.begin(kNs, /*readOnly=*/false)) return gh::domain::ErrorCode::ConfigStoreFailed;
    p.putUChar(kKey, 0);
    p.end();
    return gh::domain::ErrorCode::Ok;
}

}  // namespace gh::infra
