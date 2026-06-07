#pragma once
#ifdef ARDUINO

#include <ESPAsyncWebServer.h>
#include "ports/IAutoWaterConfigStore.hpp"
#include "ports/IMqttCredsStore.hpp"
#include "ports/IWifiCredsStore.hpp"

namespace gh::presentation {

class RestConfigRoutes {
public:
    RestConfigRoutes(gh::domain::IAutoWaterConfigStore& auto_water_store,
                       gh::domain::IMqttCredsStore&       mqtt_store,
                       gh::domain::IWifiCredsStore&       wifi_store,
                       AsyncAuthenticationMiddleware&     auth) noexcept
        : auto_water_store_{auto_water_store}, mqtt_store_{mqtt_store},
          wifi_store_{wifi_store}, auth_{auth} {}

    void registerOn(AsyncWebServer&) noexcept;

private:
    gh::domain::IAutoWaterConfigStore& auto_water_store_;
    gh::domain::IMqttCredsStore&       mqtt_store_;
    gh::domain::IWifiCredsStore&       wifi_store_;
    AsyncAuthenticationMiddleware&     auth_;
};

}  // namespace gh::presentation

#endif  // ARDUINO
