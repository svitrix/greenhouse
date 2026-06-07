#pragma once
#include <cstdint>
#include <cstring>

namespace gh::domain {
struct WifiCreds {
    char ssid[33];        // 32 + NUL
    char password[65];    // 64 + NUL (WPA2 max)
    char hostname[33];    // 32 + NUL (DNS limit per label)

    [[nodiscard]] bool valid() const noexcept {
        return ssid[0] != '\0' && std::strlen(ssid) <= 32
            && std::strlen(password) <= 64
            && std::strlen(hostname) <= 32;
    }
};
}
