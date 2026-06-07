#include "IrrigationService.hpp"
#include "entities/PumpState.hpp"

namespace gh::app {

const char* outcomeCode(AutoWaterOutcome o) noexcept {
    switch (o) {
        case AutoWaterOutcome::Started:                 return "start";
        case AutoWaterOutcome::SkipAboveThreshold:      return "skip:above_threshold";
        case AutoWaterOutcome::LockNoFreshSoil:         return "lock:no_fresh_soil";
        case AutoWaterOutcome::LockInsufficientSources: return "lock:insufficient_sources";
        case AutoWaterOutcome::LockMaxRuntime:          return "lock:max_runtime";
        case AutoWaterOutcome::LockMinInterval:         return "lock:min_interval";
        case AutoWaterOutcome::LockFloatSwitch:         return "lock:float_switch";
        case AutoWaterOutcome::Disabled:                return "disabled";
    }
    return "unknown";
}

IrrigationService::IrrigationService(
    gh::domain::INodeRegistry& reg, gh::domain::IPump& pump,
    gh::domain::IFloatSwitch& fsw, gh::domain::IClock& clock,
    gh::domain::ILogger& log, gh::domain::AutoWaterConfig cfg, uint32_t max_runtime_ms) noexcept
    : reg_{reg}, pump_{pump}, float_switch_{fsw}, clock_{clock}, log_{log},
      cfg_{cfg}, max_runtime_ms_{max_runtime_ms} {}

AutoWaterDecision IrrigationService::tick() noexcept {
    AutoWaterDecision d{};
    d.monotonic_ms = clock_.nowMs();

    // 1. Watchdog: if pump is on and we've exceeded max_runtime_ms_, stop.
    if (pump_.state() == gh::domain::PumpState::On &&
        (d.monotonic_ms - pump_started_ms_) >= max_runtime_ms_) {
        (void)pump_.turnOff();
        d.outcome = AutoWaterOutcome::LockMaxRuntime;
        log_.warn("irrigation", "max_runtime watchdog: pump forced off");
        last_ = d;
        return d;
    }

    if (!cfg_.enabled) {
        d.outcome = AutoWaterOutcome::Disabled;
        last_ = d;
        return d;
    }

    // 2. Gather fresh / stale soil sources.
    const uint32_t stale_ms = static_cast<uint32_t>(cfg_.stale_threshold_s) * 1000u;
    float sum = 0.0f;
    uint8_t fresh_count = 0;
    for (const auto& snap : reg_.snapshotAll()) {
        for (const auto& s : snap.samples) {
            if (s.quantity != gh::protocol::Quantity::SoilMoisturePct) continue;
            const uint32_t age = d.monotonic_ms - s.monotonic_ms;
            if (age <= stale_ms && snap.online) {
                sum += s.value_si;
                ++fresh_count;
                d.fresh_sources.push_back(snap.id);
            } else {
                d.stale_sources.push_back(snap.id);
            }
        }
    }

    if (fresh_count == 0) {
        d.outcome = AutoWaterOutcome::LockNoFreshSoil;
        last_ = d;
        return d;
    }
    if (fresh_count < cfg_.min_fresh_sources) {
        d.outcome = AutoWaterOutcome::LockInsufficientSources;
        last_ = d;
        return d;
    }

    const float avg = sum / static_cast<float>(fresh_count);
    d.avg_moisture_pct = avg;

    if (avg >= static_cast<float>(cfg_.trigger_below_pct)) {
        d.outcome = AutoWaterOutcome::SkipAboveThreshold;
        last_ = d;
        return d;
    }

    // 3. Float-switch and min-interval gates.
    if (!float_switch_.hasWater()) {
        d.outcome = AutoWaterOutcome::LockFloatSwitch;
        last_ = d;
        return d;
    }
    if (last_run_ms_ != 0 &&
        (d.monotonic_ms - last_run_ms_) <
        static_cast<uint32_t>(cfg_.min_interval_min) * 60u * 1000u) {
        d.outcome = AutoWaterOutcome::LockMinInterval;
        last_ = d;
        return d;
    }

    // 4. Start pump (only if not already running — manual requestOn may have done so).
    if (pump_.state() != gh::domain::PumpState::On) {
        if (pump_.turnOn() == gh::domain::ErrorCode::Ok) {
            pump_started_ms_ = d.monotonic_ms;
            last_run_ms_     = d.monotonic_ms;
            d.outcome        = AutoWaterOutcome::Started;
            log_.info("irrigation", "pump started by auto-water");
        } else {
            d.outcome = AutoWaterOutcome::LockMaxRuntime;
            log_.error("irrigation", "pump driver refused turnOn()");
        }
    } else {
        d.outcome = AutoWaterOutcome::Started;  // already running, count as started
    }

    last_ = d;
    return d;
}

AutoWaterDecision IrrigationService::requestOn() noexcept {
    AutoWaterDecision d{};
    d.monotonic_ms = clock_.nowMs();

    if (pump_.state() == gh::domain::PumpState::On) {
        // Already running — treat as a no-op success.
        d.outcome = AutoWaterOutcome::Started;
        last_ = d;
        return d;
    }
    if (!float_switch_.hasWater()) {
        d.outcome = AutoWaterOutcome::LockFloatSwitch;
        last_ = d;
        return d;
    }
    if (pump_.turnOn() == gh::domain::ErrorCode::Ok) {
        pump_started_ms_ = d.monotonic_ms;
        last_run_ms_     = d.monotonic_ms;
        d.outcome        = AutoWaterOutcome::Started;
        log_.info("irrigation", "pump started by manual request");
    } else {
        d.outcome = AutoWaterOutcome::LockMaxRuntime;
        log_.error("irrigation", "manual turnOn refused");
    }
    last_ = d;
    return d;
}

AutoWaterDecision IrrigationService::requestOff() noexcept {
    AutoWaterDecision d{};
    d.monotonic_ms = clock_.nowMs();
    d.outcome      = AutoWaterOutcome::Started;
    (void)pump_.turnOff();
    pump_started_ms_ = 0;
    last_ = d;
    log_.info("irrigation", "pump stopped by manual request");
    return d;
}

}
