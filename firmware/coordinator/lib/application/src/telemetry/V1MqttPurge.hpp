#pragma once
#include "ports/IMqttClient.hpp"

namespace gh::app {

class V1MqttPurge {
public:
    // Returns true if the purge was performed OR was already done previously.
    // Returns false if MQTT is not connected yet — caller retries next tick.
    static bool runIfNeeded(gh::domain::IMqttClient&, const char* device_id) noexcept;
};

}
