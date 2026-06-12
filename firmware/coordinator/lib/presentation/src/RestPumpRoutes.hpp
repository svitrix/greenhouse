#pragma once
#ifdef ARDUINO

#include <ESPAsyncWebServer.h>
#include "irrigation/IrrigationService.hpp"
#include "ports/IPump.hpp"
#include "RestHelpers.hpp"

namespace gh::presentation {

class RestPumpRoutes {
public:
    RestPumpRoutes(gh::app::IrrigationService&  irrigation,
                     gh::domain::IPump&             pump) noexcept
        : irrigation_{irrigation}, pump_{pump} {}

    void registerOn(AsyncWebServer&) noexcept;

private:
    gh::app::IrrigationService&  irrigation_;
    gh::domain::IPump&             pump_;
    char                           body_buf_[rest::kMaxBodyBytes] = {};
};

}  // namespace gh::presentation

#endif  // ARDUINO
