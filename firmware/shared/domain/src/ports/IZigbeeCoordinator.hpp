#pragma once
#include <cstdint>
#include "errors/ErrorCode.hpp"

namespace gh::domain {

class IZigbeeReportSink;   // forward declared for setReportSink

// Coordinator-side Zigbee adapter. Owns the network, routes inbound reports
// to an IZigbeeReportSink (set via setReportSink). All callbacks fire from
// the Zigbee task context, not the main task.
struct IZigbeeCoordinator {
    virtual ~IZigbeeCoordinator() = default;

    // start() — non-blocking: creates the Zigbee task, forms the network, opens
    // permit-join for initial_permit_ms on first boot.
    [[nodiscard]] virtual ErrorCode start(uint32_t initial_permit_ms) noexcept = 0;

    // Writes attribute 0xFF00 (report period seconds) to a sensor at short_addr.
    // Returns Ok on successful enqueue (not delivery confirmation).
    // Returns NetworkDown on SDK error or lock timeout, InvalidArgument on
    // out-of-range period_s.
    [[nodiscard]] virtual ErrorCode
    writeReportPeriod(uint16_t short_addr, uint32_t period_s) noexcept = 0;

    // Opens the Zigbee permit-join window for duration_s seconds. duration_s
    // must be in [1, 254] per ZigBee BDB. Returns Ok on enqueue; NetworkDown
    // on SDK error or lock timeout; InvalidArgument otherwise.
    [[nodiscard]] virtual ErrorCode
    openPermitJoin(uint16_t duration_s) noexcept = 0;

    // Multi-node sink. ZigbeeCoordinatorAdapter routes presence + channel reports
    // here. Must be set before start() to avoid dropping early frames.
    virtual void setReportSink(IZigbeeReportSink* sink) noexcept = 0;

    // Issues Mgmt_Leave_req to the given short_addr. Returns ErrorCode::Timeout if
    // the device did not acknowledge within ~5s. The caller still proceeds with
    // local cleanup (alias / history / registry) regardless of ack.
    [[nodiscard]] virtual gh::domain::ErrorCode
        requestLeave(uint16_t short_addr, uint64_t ieee) noexcept = 0;
};

}
