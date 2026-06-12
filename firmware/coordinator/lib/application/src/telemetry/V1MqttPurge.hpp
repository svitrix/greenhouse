#pragma once
#include "ports/IMqttClient.hpp"
#include "ports/IOneShotFlagStore.hpp"

namespace gh::app {

class V1MqttPurge {
public:
    explicit V1MqttPurge(gh::domain::IOneShotFlagStore& flags) noexcept : flags_{flags} {}

    // Returns true if the purge was performed OR was already done previously.
    // Returns false if MQTT is not connected yet — caller retries next tick.
    [[nodiscard]] bool runIfNeeded(gh::domain::IMqttClient& mqtt,
                                   const char* device_id) noexcept;

private:
    gh::domain::IOneShotFlagStore& flags_;

    static constexpr const char* kFlagKey = "mqtt_purge_v1";
};

}
