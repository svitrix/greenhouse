#include "NvsOneShotFlagStore.hpp"

namespace gh::infra {

bool NvsOneShotFlagStore::isSet(const char* key) noexcept {
    Preferences p;
    if (!p.begin(kNs, /*readOnly=*/true)) return false;
    bool v = p.getBool(key, false);
    p.end();
    return v;
}

bool NvsOneShotFlagStore::set(const char* key) noexcept {
    Preferences p;
    if (!p.begin(kNs, /*readOnly=*/false)) return false;
    bool ok = p.putBool(key, true);
    p.end();
    return ok;
}

}
