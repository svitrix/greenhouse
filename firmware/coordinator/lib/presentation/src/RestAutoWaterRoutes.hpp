#pragma once
#ifdef ARDUINO

#include <ESPAsyncWebServer.h>
#include "irrigation/IrrigationService.hpp"

namespace gh::presentation {

class RestAutoWaterRoutes {
public:
    RestAutoWaterRoutes(gh::app::IrrigationService&  irrigation,
                          AsyncAuthenticationMiddleware& auth) noexcept
        : irrigation_{irrigation}, auth_{auth} {}

    void registerOn(AsyncWebServer&) noexcept;

private:
    gh::app::IrrigationService&  irrigation_;
    AsyncAuthenticationMiddleware& auth_;
};

}  // namespace gh::presentation

#endif  // ARDUINO
