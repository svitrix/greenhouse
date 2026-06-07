#pragma once
#include "AutoWaterDecision.hpp"
#include "entities/AutoWaterConfig.hpp"
#include "ports/IClock.hpp"
#include "ports/IFloatSwitch.hpp"
#include "ports/ILogger.hpp"
#include "ports/INodeRegistry.hpp"
#include "ports/IPump.hpp"

namespace gh::app {

class IrrigationService {
public:
    IrrigationService(gh::domain::INodeRegistry& reg,
                        gh::domain::IPump&         pump,
                        gh::domain::IFloatSwitch&  float_switch,
                        gh::domain::IClock&        clock,
                        gh::domain::ILogger&       log,
                        gh::domain::AutoWaterConfig cfg,
                        uint32_t                   max_runtime_ms) noexcept;

    AutoWaterDecision tick() noexcept;

    [[nodiscard]] AutoWaterDecision requestOn () noexcept;
    [[nodiscard]] AutoWaterDecision requestOff() noexcept;

    [[nodiscard]] const AutoWaterDecision& lastDecision() const noexcept { return last_; }

    void setConfig(gh::domain::AutoWaterConfig cfg) noexcept { cfg_ = cfg; }

private:
    gh::domain::INodeRegistry&  reg_;
    gh::domain::IPump&          pump_;
    gh::domain::IFloatSwitch&   float_switch_;
    gh::domain::IClock&         clock_;
    gh::domain::ILogger&        log_;
    gh::domain::AutoWaterConfig cfg_;
    uint32_t                    max_runtime_ms_;
    uint32_t                    pump_started_ms_ = 0;
    uint32_t                    last_run_ms_     = 0;
    AutoWaterDecision           last_{};
};

}
