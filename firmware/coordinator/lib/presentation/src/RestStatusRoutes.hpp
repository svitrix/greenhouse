#pragma once
#ifdef ARDUINO

#include <ESPAsyncWebServer.h>
#include "ports/IClock.hpp"
#include "ports/IMqttClient.hpp"
#include "ports/INodeRegistry.hpp"
#include "ports/ISystemInfo.hpp"

namespace gh::presentation {

class RestStatusRoutes {
public:
    RestStatusRoutes(gh::domain::INodeRegistry&     reg,
                       gh::domain::IClock&            clock,
                       gh::domain::IMqttClient&       mqtt,
                       gh::domain::ISystemInfo&       sysinfo,
                       const char*                    device_id,
                       AsyncAuthenticationMiddleware& auth) noexcept
        : reg_{reg}, clock_{clock}, mqtt_{mqtt}, sysinfo_{sysinfo},
          device_id_{device_id}, auth_{auth} {}

    void registerOn(AsyncWebServer&) noexcept;

private:
    gh::domain::INodeRegistry&     reg_;
    gh::domain::IClock&            clock_;
    gh::domain::IMqttClient&       mqtt_;
    gh::domain::ISystemInfo&       sysinfo_;
    const char*                    device_id_;
    AsyncAuthenticationMiddleware& auth_;
};

}  // namespace gh::presentation

#endif  // ARDUINO
