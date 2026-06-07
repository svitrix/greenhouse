#include "NvsProvisioningFlagStore.hpp"

namespace gh::infra {

bool NvsProvisioningFlagStore::isForced() noexcept {
    Preferences p;
    if (!p.begin(kNs, /*readOnly=*/true)) return false;
    bool v = p.getBool(kKey, false);
    p.end();
    return v;
}

gh::domain::ErrorCode NvsProvisioningFlagStore::setForced(bool v) noexcept {
    Preferences p;
    if (!p.begin(kNs, /*readOnly=*/false)) return gh::domain::ErrorCode::ConfigStoreFailed;
    p.putBool(kKey, v);
    p.end();
    return gh::domain::ErrorCode::Ok;
}

}
