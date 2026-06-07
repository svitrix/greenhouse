#include "NvsZigbeeNetStore.hpp"
#include <Preferences.h>
#include <esp_random.h>

namespace gh::infra {

namespace {
constexpr const char* kNamespace = "zigbee_net";
constexpr const char* kKey       = "extpanid";

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
    if (prefs.begin(kNamespace, /*readOnly=*/false)) {
        prefs.putBytes(kKey, out.data(), out.size());
        prefs.end();
    }
    return out;
}

}  // namespace gh::infra
