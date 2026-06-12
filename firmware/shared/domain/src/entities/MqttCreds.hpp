#pragma once
#include <cstdint>
#include <cstring>

namespace gh::domain {

// Bump on any on-flash layout change. First struct byte; checked on load.
inline constexpr uint8_t kMqttCredsSchemaVersion = 1;

struct MqttCreds {
    uint8_t  schema_version = kMqttCredsSchemaVersion;  // MUST be first member
    char     host[64];
    uint16_t port;
    char     user[33];
    char     password[65];
    char     client_id[33];
    char     topic_prefix[33];

    // Force-terminate every char[] so a corrupt / truncated NVS record can
    // never cause an over-read downstream. Call right after a raw load.
    void normalizeForStorage() noexcept {
        host[sizeof(host) - 1]                 = '\0';
        user[sizeof(user) - 1]                 = '\0';
        password[sizeof(password) - 1]         = '\0';
        client_id[sizeof(client_id) - 1]       = '\0';
        topic_prefix[sizeof(topic_prefix) - 1] = '\0';
    }

    [[nodiscard]] bool valid() const noexcept {
        return hasNulWithinBounds()
            && host[0] != '\0'
            && port != 0
            && std::strlen(host) <= 63
            && std::strlen(user) <= 32
            && std::strlen(password) <= 64
            && std::strlen(client_id) <= 32
            && std::strlen(topic_prefix) <= 32;
    }

private:
    [[nodiscard]] bool hasNulWithinBounds() const noexcept {
        return std::memchr(host, '\0', sizeof(host)) != nullptr
            && std::memchr(user, '\0', sizeof(user)) != nullptr
            && std::memchr(password, '\0', sizeof(password)) != nullptr
            && std::memchr(client_id, '\0', sizeof(client_id)) != nullptr
            && std::memchr(topic_prefix, '\0', sizeof(topic_prefix)) != nullptr;
    }
};
}
