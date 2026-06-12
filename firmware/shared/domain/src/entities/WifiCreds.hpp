#pragma once
#include <cstdint>
#include <cstring>

namespace gh::domain {

// Bump whenever the on-flash layout of WifiCreds changes (field added /
// resized / reordered). Stored as the first struct byte so a load can reject
// a record written by a different firmware build instead of blitting garbage.
inline constexpr uint8_t kWifiCredsSchemaVersion = 1;

struct WifiCreds {
    uint8_t schema_version = kWifiCredsSchemaVersion;  // MUST be first member
    char    ssid[33];        // 32 + NUL
    char    password[65];    // 64 + NUL (WPA2 max)
    char    hostname[33];    // 32 + NUL (DNS limit per label)

    // Force-terminate every char[] so a corrupt / truncated NVS record can
    // never cause an over-read in WiFi.begin(ssid) etc. Call right after a
    // raw load, before valid().
    void normalizeForStorage() noexcept {
        ssid[sizeof(ssid) - 1]         = '\0';
        password[sizeof(password) - 1] = '\0';
        hostname[sizeof(hostname) - 1] = '\0';
    }

    [[nodiscard]] bool valid() const noexcept {
        return hasNulWithinBounds()
            && ssid[0] != '\0' && std::strlen(ssid) <= 32
            && std::strlen(password) <= 64
            && std::strlen(hostname) <= 32;
    }

private:
    [[nodiscard]] bool hasNulWithinBounds() const noexcept {
        return std::memchr(ssid, '\0', sizeof(ssid)) != nullptr
            && std::memchr(password, '\0', sizeof(password)) != nullptr
            && std::memchr(hostname, '\0', sizeof(hostname)) != nullptr;
    }
};
}
