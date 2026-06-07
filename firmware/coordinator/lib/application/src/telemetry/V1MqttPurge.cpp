#include "V1MqttPurge.hpp"
#ifdef ARDUINO
#include <Preferences.h>
#include <cstdio>
#include <string_view>

namespace gh::app {

bool V1MqttPurge::runIfNeeded(gh::domain::IMqttClient& mqtt,
                                const char* device_id) noexcept
{
    Preferences prefs;
    if (!prefs.begin("nvs_flags", false)) return false;
    if (prefs.getBool("mqtt_purge_v1", false)) {
        prefs.end();
        return true;
    }
    if (!mqtt.isConnected()) {
        prefs.end();
        return false;
    }

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
    prefs.putBool("mqtt_purge_v1", true);
    prefs.end();
    return true;
}

}
#else
// Stub for host builds — V1MqttPurge::runIfNeeded() is wired only at runtime
// on the device; the host test env doesn't link this method.
namespace gh::app {
bool V1MqttPurge::runIfNeeded(gh::domain::IMqttClient&, const char*) noexcept {
    return true;
}
}
#endif
