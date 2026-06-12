#include "NvsAnalyticsConfigStore.hpp"
#include <cstring>

namespace gh::infra {

using gh::domain::AnalyticsConfig;
using gh::domain::ErrorCode;

namespace {
constexpr uint32_t kDefaultFlushPeriodS = 900;  // 15 min
}

ErrorCode NvsAnalyticsConfigStore::load(AnalyticsConfig& out) noexcept {
    // Error codes unified with the other Nvs*Stores (ConfigStoreFailed on open
    // failure, ConfigNotFound when no record yet) instead of the one-off
    // NvsAccessFailed / NotFound this store used to return.
    if (!prefs_.begin(kNamespace, /*readOnly=*/true)) {
        return ErrorCode::ConfigStoreFailed;
    }
    std::memset(&out, 0, sizeof(out));

    prefs_.getString("url", out.backend_url, sizeof(out.backend_url));
    prefs_.getString("key", out.api_key,     sizeof(out.api_key));
    out.flush_period_s = prefs_.getUInt("period_s", kDefaultFlushPeriodS);
    out.insecure_tls   = prefs_.getBool("insecure", false);
    prefs_.end();

    // getString leaves the buffer NUL-terminated; guard against a stored value
    // that filled the buffer without room for the terminator.
    out.backend_url[sizeof(out.backend_url) - 1] = '\0';
    out.api_key[sizeof(out.api_key) - 1]         = '\0';

    if (out.backend_url[0] == '\0') return ErrorCode::ConfigNotFound;
    return ErrorCode::Ok;
}

ErrorCode NvsAnalyticsConfigStore::save(const AnalyticsConfig& in) noexcept {
    // Reject silent truncation: backend_url / api_key are fixed-size fields, so
    // an over-long (unterminated) C-string would be cut to fit and a wrong URL
    // persisted. A NUL within bounds proves the source fits.
    if (std::memchr(in.backend_url, '\0', sizeof(in.backend_url)) == nullptr ||
        std::memchr(in.api_key,     '\0', sizeof(in.api_key))     == nullptr) {
        return ErrorCode::ValidationFailed;
    }
    if (!prefs_.begin(kNamespace, /*readOnly=*/false)) {
        return ErrorCode::ConfigStoreFailed;
    }
    // putString writes strlen() bytes; an empty value legitimately writes 0, so
    // only a short write relative to the source length signals a real failure.
    bool ok = true;
    ok &= prefs_.putString("url", in.backend_url) == std::strlen(in.backend_url);
    ok &= prefs_.putString("key", in.api_key)     == std::strlen(in.api_key);
    ok &= prefs_.putUInt("period_s", in.flush_period_s) != 0;
    // putBool(false) still writes 1 byte, so a false value is not a failure.
    prefs_.putBool("insecure", in.insecure_tls);
    prefs_.end();
    return ok ? ErrorCode::Ok : ErrorCode::ConfigStoreFailed;
}

void NvsAnalyticsConfigStore::clear() noexcept {
    if (prefs_.begin(kNamespace, /*readOnly=*/false)) {
        prefs_.clear();
        prefs_.end();
    }
}

}
