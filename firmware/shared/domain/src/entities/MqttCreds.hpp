#pragma once
#include <cstdint>
#include <cstring>

namespace gh::domain {
struct MqttCreds {
    char     host[64];
    uint16_t port;
    char     user[33];
    char     password[65];
    char     client_id[33];
    char     topic_prefix[33];

    [[nodiscard]] bool valid() const noexcept {
        return host[0] != '\0'
            && port != 0
            && std::strlen(host) <= 63
            && std::strlen(user) <= 32
            && std::strlen(password) <= 64
            && std::strlen(client_id) <= 32
            && std::strlen(topic_prefix) <= 32;
    }
};
}
