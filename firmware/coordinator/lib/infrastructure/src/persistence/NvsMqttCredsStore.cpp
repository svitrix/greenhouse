#include "NvsMqttCredsStore.hpp"

namespace gh::infra {

gh::domain::ErrorCode NvsMqttCredsStore::begin() noexcept {
    if (!prefs_.begin("mqtt", false)) {
        return gh::domain::ErrorCode::ConfigStoreFailed;
    }
    opened_ = true;
    return gh::domain::ErrorCode::Ok;
}

gh::domain::Result<gh::domain::MqttCreds>
NvsMqttCredsStore::load() noexcept {
    using R = gh::domain::Result<gh::domain::MqttCreds>;
    if (!opened_) return R::failure(gh::domain::ErrorCode::SensorNotReady);

    gh::domain::MqttCreds out{};
    const size_t got = prefs_.getBytes("v1", &out, sizeof(out));
    // Size mismatch covers both "no record yet" and a legacy record written by
    // an older firmware whose struct lacked schema_version — treated as absent
    // (caller re-provisions; next save() writes the versioned layout).
    if (got != sizeof(out)) {
        return R::failure(gh::domain::ErrorCode::ConfigNotFound);
    }
    if (out.schema_version != gh::domain::kMqttCredsSchemaVersion) {
        return R::failure(gh::domain::ErrorCode::SensorVersionMismatch);
    }
    out.normalizeForStorage();
    if (!out.valid()) {
        return R::failure(gh::domain::ErrorCode::SensorOutOfRange);
    }
    return R::success(out);
}

gh::domain::ErrorCode
NvsMqttCredsStore::save(gh::domain::MqttCreds creds) noexcept {
    if (!opened_)        return gh::domain::ErrorCode::SensorNotReady;
    creds.schema_version = gh::domain::kMqttCredsSchemaVersion;
    creds.normalizeForStorage();
    if (!creds.valid())  return gh::domain::ErrorCode::SensorOutOfRange;

    const size_t written = prefs_.putBytes("v1", &creds, sizeof(creds));
    if (written != sizeof(creds)) {
        return gh::domain::ErrorCode::ConfigStoreFailed;
    }
    return gh::domain::ErrorCode::Ok;
}

}
