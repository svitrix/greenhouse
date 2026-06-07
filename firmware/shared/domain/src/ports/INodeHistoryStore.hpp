#pragma once
#include <cstdint>
#include <etl/vector.h>
#include "entities/NodeId.hpp"
#include "entities/SensorKind.hpp"
#include "Quantity.hpp"

namespace gh::domain {

// Ring-buffer depth per (node, channel) series. Drives coordinator static RAM:
// kMaxRegisteredNodes(8) * 8 channels * this * sizeof(Point=8B). At 128 the
// history store is ~69 KB (vs ~131 KB at 256). On-device history only backs the
// live SPA chart — long-term storage lives in the cloud hub — so a short window
// is fine: at the 60 s default report period 128 points ≈ 2.1 h of live graph.
inline constexpr size_t kHistoryMaxPointsPerSeries = 128;

class INodeHistoryStore {
public:
    struct Point {
        uint32_t monotonic_ms;
        float    value;
    };

    virtual ~INodeHistoryStore() = default;

    virtual void recordPoint(NodeId, SensorKind, gh::protocol::Quantity, Point) noexcept = 0;
    virtual void forgetNode (NodeId)                                            noexcept = 0;

    [[nodiscard]] virtual etl::vector<Point, kHistoryMaxPointsPerSeries>
        query(NodeId, SensorKind, gh::protocol::Quantity,
              uint32_t since_monotonic_ms) const noexcept = 0;
};

}
