#include "NvsZigbeeNetStore.hpp"
#include <Preferences.h>
#include <esp_random.h>
#include <esp_log.h>

namespace gh::infra {

namespace {
constexpr const char* kNamespace = "zigbee_net";
constexpr const char* kKey       = "extpanid";
constexpr const char* kLogTag    = "zigbee_net";
constexpr uint8_t     kPersistRetries = 3;

// A freshly generated ExtPanId that is never persisted means the coordinator
// forms a brand-new network on every boot, silently dropping all paired
// sensor-nodes. Retry the write a few times and log loudly on failure so the
// operator can see why pairings keep resetting.
[[nodiscard]] bool persist(const std::array<uint8_t, 8>& v) noexcept {
    for (uint8_t attempt = 0; attempt < kPersistRetries; ++attempt) {
        Preferences prefs;
        if (prefs.begin(kNamespace, /*readOnly=*/false)) {
            const size_t wrote = prefs.putBytes(kKey, v.data(), v.size());
            prefs.end();
            if (wrote == v.size()) return true;
        }
    }
    return false;
}

[[nodiscard]] bool isInvalid(const std::array<uint8_t, 8>& v) noexcept {
    bool all_zero = true;
    bool all_ff   = true;
    for (auto b : v) {
        if (b != 0x00) all_zero = false;
        if (b != 0xFF) all_ff   = false;
    }
    return all_zero || all_ff;
}
}  // namespace

std::array<uint8_t, 8> NvsZigbeeNetStore::loadOrGenerate() noexcept {
    Preferences prefs;
    std::array<uint8_t, 8> out{};
    if (prefs.begin(kNamespace, /*readOnly=*/true)) {
        const size_t got = prefs.getBytes(kKey, out.data(), out.size());
        prefs.end();
        if (got == out.size() && !isInvalid(out)) {
            return out;
        }
    }
    // First boot (or corrupt entry): generate fresh.
    do {
        esp_fill_random(out.data(), out.size());
    } while (isInvalid(out));
    if (!persist(out)) {
        ESP_LOGW(kLogTag,
                 "failed to persist generated ExtPanId after %u attempts - "
                 "Zigbee pairings will reset on next boot",
                 static_cast<unsigned>(kPersistRetries));
    }
    return out;
}

}  // namespace gh::infra
