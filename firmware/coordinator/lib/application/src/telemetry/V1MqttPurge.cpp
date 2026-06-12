#include "V1MqttPurge.hpp"
#include <cstdio>
#include <string_view>

namespace gh::app {

bool V1MqttPurge::runIfNeeded(gh::domain::IMqttClient& mqtt,
                              const char* device_id) noexcept
{
    if (flags_.isSet(kFlagKey)) return true;
    if (!mqtt.isConnected()) return false;

    static const char* kV1Topics[] = {
        "homeassistant/sensor/greenhouse_%s_air_temperature/config",
        "homeassistant/sensor/greenhouse_%s_air_humidity/config",
        "homeassistant/sensor/greenhouse_%s_soil_moisture/config",
        "homeassistant/sensor/greenhouse_%s_soil_temperature/config",
        "homeassistant/sensor/greenhouse_%s_battery_pct/config",
        "homeassistant/sensor/greenhouse_%s_battery_voltage/config",
        "homeassistant/switch/greenhouse_%s_pump/config",
        "greenhouse/%s/air/temperature",
        "greenhouse/%s/air/humidity",
        "greenhouse/%s/soil/moisture",
        "greenhouse/%s/soil/temperature",
    };
    for (const char* fmt : kV1Topics) {
        char topic[160];
        std::snprintf(topic, sizeof(topic), fmt, device_id);
        (void)mqtt.publish(topic, std::string_view{""}, /*retain*/ true);
    }
    return flags_.set(kFlagKey);
}

}
