#pragma once
#include <array>
#include <cstdint>
#include "ports/IZigbeeCoordinator.hpp"
#include "ports/IZigbeeReportSink.hpp"

namespace gh::infra {

// Zigbee Coordinator adapter for the coordinator firmware.
//
// Implements IZigbeeCoordinator using espressif esp-zigbee-lib (already
// bundled in the pioarduino framework-arduinoespressif32-libs package for
// esp32c6 — no extra lib_deps entry needed).
//
// start() initialises the Zigbee stack as a Coordinator, launches the
// Zigbee main-loop task, and opens a permit-join window of
// initial_permit_ms milliseconds once the network has formed.
//
// Incoming ZCL Report Attributes commands from sensor-nodes are decoded
// in the static action handler and dispatched to the IZigbeeReportSink
// set via setReportSink. Each attribute emits independently — there is
// no per-source staging; the report router on the application side
// reassembles into NodeView reads as needed.

class ZigbeeCoordinatorAdapter final : public gh::domain::IZigbeeCoordinator {
public:
    explicit ZigbeeCoordinatorAdapter(const std::array<uint8_t, 8>& ext_pan_id) noexcept;

    void setReportSink(gh::domain::IZigbeeReportSink* sink) noexcept override;

    [[nodiscard]] gh::domain::ErrorCode start(uint32_t initial_permit_ms) noexcept override;
    [[nodiscard]] gh::domain::ErrorCode openPermitJoin(uint16_t duration_s) noexcept override;
    [[nodiscard]] gh::domain::ErrorCode requestLeave(uint16_t short_addr,
                                                       uint64_t ieee) noexcept override;
    [[nodiscard]] gh::domain::ErrorCode writeReportPeriod(uint16_t short_addr,
                                                            uint32_t period_s) noexcept override;
    uint8_t drainPendingPeriodWrites(uint32_t period_s) noexcept;

    // True while the permit-join window opened by formation or openPermitJoin()
    // is still active. Used by the status LED to show "ready to pair".
    [[nodiscard]] bool isPermitJoinOpen() const noexcept;

private:
    std::array<uint8_t, 8> ext_pan_id_;
};

}  // namespace gh::infra
