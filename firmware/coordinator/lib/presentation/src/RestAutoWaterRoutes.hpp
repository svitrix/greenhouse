#pragma once
#ifdef ARDUINO

#include <ESPAsyncWebServer.h>
#include "irrigation/IrrigationService.hpp"

namespace gh::presentation {

class RestAutoWaterRoutes {
public:
    explicit RestAutoWaterRoutes(gh::app::IrrigationService& irrigation) noexcept
        : irrigation_{irrigation} {}

    void registerOn(AsyncWebServer&) noexcept;

private:
    gh::app::IrrigationService&  irrigation_;
};

}  // namespace gh::presentation

#endif  // ARDUINO
