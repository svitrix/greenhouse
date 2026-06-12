#pragma once
#ifdef ARDUINO

#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "ports/IAutoWaterConfigStore.hpp"
#include "ports/IMqttCredsStore.hpp"
#include "ports/IWifiCredsStore.hpp"
#include "RestHelpers.hpp"

namespace gh::presentation {

class RestConfigRoutes {
public:
    RestConfigRoutes(gh::domain::IAutoWaterConfigStore& auto_water_store,
                       gh::domain::IMqttCredsStore&       mqtt_store,
                       gh::domain::IWifiCredsStore&       wifi_store) noexcept
        : auto_water_store_{auto_water_store}, mqtt_store_{mqtt_store},
          wifi_store_{wifi_store} {}

    void registerOn(AsyncWebServer&) noexcept;

private:
    // Validate + persist one config section; on failure they emit the error
    // response themselves and return false so the handler stops.
    [[nodiscard]] bool applyAutoWater(AsyncWebServerRequest*, JsonObjectConst) noexcept;
    [[nodiscard]] bool applyMqtt(AsyncWebServerRequest*, JsonObjectConst) noexcept;

    gh::domain::IAutoWaterConfigStore& auto_water_store_;
    gh::domain::IMqttCredsStore&       mqtt_store_;
    gh::domain::IWifiCredsStore&       wifi_store_;
    char                               body_buf_[rest::kMaxBodyBytes] = {};
};

}  // namespace gh::presentation

#endif  // ARDUINO
