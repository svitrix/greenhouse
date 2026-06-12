#pragma once
#include <cstdint>
#include <optional>
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

    [[nodiscard]] AutoWaterDecision tick() noexcept;

    [[nodiscard]] AutoWaterDecision requestOn () noexcept;
    [[nodiscard]] AutoWaterDecision requestOff() noexcept;

    // Independent safety backstop. Designed to be callable from a dedicated
    // high-priority RTOS task / esp_timer (see composition root) that does NOT
    // share the network-blocking telemetry task. Latches SafetyLocked if the
    // pump is running past max_runtime_ms_ or the float switch reports a dry
    // tank. Returns true when it forced a cutoff. Re-entrant-safe: depends only
    // on the injected ports and pump_started_ms_.
    bool enforceSafetyCutoff(uint32_t now_ms) noexcept;

    [[nodiscard]] const AutoWaterDecision& lastDecision() const noexcept { return last_; }

    void setConfig(gh::domain::AutoWaterConfig cfg) noexcept { cfg_ = cfg; }

private:
    void gatherFreshSoil(uint32_t now_ms, AutoWaterDecision& d,
                         float& sum, uint8_t& fresh_count) const noexcept;

    gh::domain::INodeRegistry&  reg_;
    gh::domain::IPump&          pump_;
    gh::domain::IFloatSwitch&   float_switch_;
    gh::domain::IClock&         clock_;
    gh::domain::ILogger&        log_;
    gh::domain::AutoWaterConfig cfg_;
    uint32_t                    max_runtime_ms_;
    std::optional<uint32_t>     pump_started_ms_{};
    std::optional<uint32_t>     last_run_ms_{};
    AutoWaterDecision           last_{};
};

}
