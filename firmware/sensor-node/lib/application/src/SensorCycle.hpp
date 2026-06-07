#pragma once
#include <cstdint>
#include "SensorRegistry.hpp"
#include "ports/IPowerRail.hpp"
#include "ports/IZigbeeEndDevice.hpp"
#include "ports/IDeepSleep.hpp"
#include "ports/IClock.hpp"
#include "ports/ILogger.hpp"

// Forward declaration — ZigbeeReportMapper lives in infrastructure. Including
// the full header here would pull the infrastructure layer into the application
// header and break the native env (which adds the infra path only to test builds).
// The full include is in SensorCycle.cpp.
namespace gh::infra { class ZigbeeReportMapper; }

namespace gh::sensor {

// One-shot cycle: power on → wait warmup → read every Ok channel → publish
// via mapper → power off → return sleep duration (ms).
class SensorCycle {
public:
    SensorCycle(gh::app::SensorRegistry&       registry,
                gh::domain::IPowerRail&        rail,
                gh::infra::ZigbeeReportMapper& mapper,
                gh::domain::IZigbeeEndDevice&  zb,
                gh::domain::IDeepSleep&        sleep,
                gh::domain::IClock&            clock,
                gh::domain::ILogger&           log,
                uint32_t                       tx_timeout_ms) noexcept;

    // Reads sensors and publishes one Zigbee report cycle.
    // Returns sleep duration in ms (= reportPeriodSeconds * 1000).
    [[nodiscard]] uint32_t runOnce() noexcept;

    [[noreturn]] void run() noexcept;

private:
    gh::app::SensorRegistry&       registry_;
    gh::domain::IPowerRail&        rail_;
    gh::infra::ZigbeeReportMapper& mapper_;
    gh::domain::IZigbeeEndDevice&  zb_;
    gh::domain::IDeepSleep&        sleep_;
    gh::domain::IClock&            clock_;
    gh::domain::ILogger&           log_;
    uint32_t                       tx_timeout_ms_;
};

}
