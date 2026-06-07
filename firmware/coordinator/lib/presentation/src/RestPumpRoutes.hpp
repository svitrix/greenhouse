#pragma once
#ifdef ARDUINO

#include <ESPAsyncWebServer.h>
#include "irrigation/IrrigationService.hpp"
#include "ports/IPump.hpp"

namespace gh::presentation {

class RestPumpRoutes {
public:
    RestPumpRoutes(gh::app::IrrigationService&  irrigation,
                     gh::domain::IPump&             pump,
                     AsyncAuthenticationMiddleware& auth) noexcept
        : irrigation_{irrigation}, pump_{pump}, auth_{auth} {}

    void registerOn(AsyncWebServer&) noexcept;

private:
    gh::app::IrrigationService&  irrigation_;
    gh::domain::IPump&             pump_;
    AsyncAuthenticationMiddleware& auth_;
};

}  // namespace gh::presentation

#endif  // ARDUINO
