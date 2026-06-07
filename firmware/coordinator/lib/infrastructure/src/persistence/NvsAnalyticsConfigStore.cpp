#include "NvsAnalyticsConfigStore.hpp"
#include <cstring>

namespace gh::infra {

using gh::domain::AnalyticsConfig;
using gh::domain::ErrorCode;

ErrorCode NvsAnalyticsConfigStore::load(AnalyticsConfig& out) noexcept {
    if (!prefs_.begin(kNamespace, /*readOnly=*/true)) {
        return ErrorCode::NvsAccessFailed;
    }
    std::memset(&out, 0, sizeof(out));

    prefs_.getString("url", out.backend_url, sizeof(out.backend_url));
    prefs_.getString("key", out.api_key,     sizeof(out.api_key));
    out.flush_period_s = prefs_.getUInt("period_s", 900);
    out.insecure_tls   = prefs_.getBool("insecure", false);
    prefs_.end();

    if (out.backend_url[0] == '\0') return ErrorCode::NotFound;
    return ErrorCode::Ok;
}

ErrorCode NvsAnalyticsConfigStore::save(const AnalyticsConfig& in) noexcept {
    if (!prefs_.begin(kNamespace, /*readOnly=*/false)) {
        return ErrorCode::NvsAccessFailed;
    }
    prefs_.putString("url", in.backend_url);
    prefs_.putString("key", in.api_key);
    prefs_.putUInt("period_s", in.flush_period_s);
    prefs_.putBool("insecure", in.insecure_tls);
    prefs_.end();
    return ErrorCode::Ok;
}

void NvsAnalyticsConfigStore::clear() noexcept {
    if (prefs_.begin(kNamespace, /*readOnly=*/false)) {
        prefs_.clear();
        prefs_.end();
    }
}

}
