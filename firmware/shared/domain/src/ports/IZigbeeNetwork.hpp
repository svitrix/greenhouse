#pragma once
#include <cstdint>
#include "entities/NodeId.hpp"
#include "errors/ErrorCode.hpp"

namespace gh::domain {

class IZigbeeNetwork {
public:
    virtual ~IZigbeeNetwork() = default;

    // duration_s in [1, 254]; out-of-range returns ErrorCode::SensorOutOfRange.
    [[nodiscard]] virtual ErrorCode permitJoin  (uint8_t duration_s) noexcept = 0;

    // Issues Mgmt_Leave_req. Returns ErrorCode::Timeout if the node does not
    // acknowledge within ~5 s; the caller may still proceed with local cleanup.
    [[nodiscard]] virtual ErrorCode requestLeave(NodeId)             noexcept = 0;
};

}
