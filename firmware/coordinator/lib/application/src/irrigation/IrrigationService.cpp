#include "IrrigationService.hpp"
#include "entities/PumpState.hpp"

namespace gh::app {

const char* outcomeCode(AutoWaterOutcome o) noexcept {
    switch (o) {
        case AutoWaterOutcome::Started:                 return "start";
        case AutoWaterOutcome::Stopped:                 return "stop";
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

bool IrrigationService::enforceSafetyCutoff(uint32_t now_ms) noexcept {
    if (pump_.state() != gh::domain::PumpState::On) return false;

    const bool max_runtime_hit =
        pump_started_ms_.has_value() &&
        (now_ms - *pump_started_ms_) >= max_runtime_ms_;
    const bool dry_tank = !float_switch_.hasWater();

    if (!max_runtime_hit && !dry_tank) return false;

    (void)pump_.lock();
    pump_started_ms_.reset();
    log_.warn("irrigation",
              max_runtime_hit ? "max_runtime watchdog: pump latched safety-locked"
                              : "dry-tank guard: pump latched safety-locked");
    return true;
}

void IrrigationService::gatherFreshSoil(uint32_t now_ms,
                                        AutoWaterDecision& d,
                                        float& sum, uint8_t& fresh_count) const noexcept {
    const uint32_t stale_ms = static_cast<uint32_t>(cfg_.stale_threshold_s) * 1000u;
    sum = 0.0f;
    fresh_count = 0;
    // One vote per node: aggregate moisture samples into a single value so a
    // node reporting several soil channels cannot multiply its weight in the
    // quorum or overflow the bounded source vectors.
    for (const auto& snap : reg_.snapshotAll()) {
        std::optional<float> node_moisture;
        uint32_t freshest_age = UINT32_MAX;
        for (const auto& s : snap.samples) {
            if (s.quantity != gh::protocol::Quantity::SoilMoisturePct) continue;
            const uint32_t age = now_ms - s.monotonic_ms;
            if (age < freshest_age) {
                freshest_age = age;
                node_moisture = s.value_si;
            }
        }
        if (!node_moisture.has_value()) continue;

        const bool fresh = (freshest_age <= stale_ms) && snap.online;
        if (fresh) {
            sum += *node_moisture;
            ++fresh_count;
            if (!d.fresh_sources.full()) d.fresh_sources.push_back(snap.id);
        } else if (!d.stale_sources.full()) {
            d.stale_sources.push_back(snap.id);
        }
    }
}

AutoWaterDecision IrrigationService::tick() noexcept {
    AutoWaterDecision d{};
    d.monotonic_ms = clock_.nowMs();

    // 1. Safety backstop runs unconditionally (before the cfg_.enabled gate)
    //    so a manually-started pump is cut off on a dry tank or max-runtime
    //    even when auto-water is disabled. [A2/A3]
    if (enforceSafetyCutoff(d.monotonic_ms)) {
        d.outcome = AutoWaterOutcome::LockMaxRuntime;
        last_ = d;
        return d;
    }

    // 2. While safety-locked, auto-water stays gated until an explicit
    //    requestOff() re-arms the pump. [A1]
    if (pump_.state() == gh::domain::PumpState::SafetyLocked) {
        d.outcome = AutoWaterOutcome::LockMaxRuntime;
        last_ = d;
        return d;
    }

    if (!cfg_.enabled) {
        d.outcome = AutoWaterOutcome::Disabled;
        last_ = d;
        return d;
    }

    float sum = 0.0f;
    uint8_t fresh_count = 0;
    gatherFreshSoil(d.monotonic_ms, d, sum, fresh_count);

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
    if (last_run_ms_.has_value() &&
        (d.monotonic_ms - *last_run_ms_) <
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

    // Safety latch gates manual start too — an explicit requestOff() must
    // re-arm the pump before it can run again. [A1]
    if (pump_.state() == gh::domain::PumpState::SafetyLocked) {
        d.outcome = AutoWaterOutcome::LockMaxRuntime;
        last_ = d;
        return d;
    }
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
    d.outcome      = AutoWaterOutcome::Stopped;
    // turnOff() de-energises AND clears any SafetyLocked latch (explicit
    // operator re-arm). [A1/A5]
    (void)pump_.turnOff();
    pump_started_ms_.reset();
    last_ = d;
    log_.info("irrigation", "pump stopped by manual request");
    return d;
}

}
