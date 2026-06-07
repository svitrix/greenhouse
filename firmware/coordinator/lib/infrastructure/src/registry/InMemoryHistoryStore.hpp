#pragma once
#include <etl/circular_buffer.h>
#include <etl/flat_map.h>
#include "ports/INodeHistoryStore.hpp"
#include "ports/INodeRegistry.hpp"

namespace gh::infra {

class InMemoryHistoryStore final : public gh::domain::INodeHistoryStore {
public:
    static constexpr uint32_t kWindowMs = 24u * 60u * 60u * 1000u;

    void recordPoint(gh::domain::NodeId, gh::domain::SensorKind,
                     gh::protocol::Quantity, Point) noexcept override;
    void forgetNode (gh::domain::NodeId) noexcept override;

    [[nodiscard]] etl::vector<Point, gh::domain::kHistoryMaxPointsPerSeries>
        query(gh::domain::NodeId, gh::domain::SensorKind,
              gh::protocol::Quantity, uint32_t since_monotonic_ms) const noexcept override;

private:
    struct Key {
        gh::domain::NodeId      node;
        gh::domain::SensorKind  kind;
        gh::protocol::Quantity  quantity;
        bool operator<(const Key& o) const noexcept {
            if (node.ieee != o.node.ieee) return node.ieee < o.node.ieee;
            if (kind     != o.kind)       return static_cast<int>(kind)     < static_cast<int>(o.kind);
            return static_cast<int>(quantity) < static_cast<int>(o.quantity);
        }
        bool operator==(const Key& o) const noexcept {
            return node == o.node && kind == o.kind && quantity == o.quantity;
        }
    };

    using Series = etl::circular_buffer<Point, gh::domain::kHistoryMaxPointsPerSeries>;
    using Map    = etl::flat_map<Key, Series,
        gh::domain::kMaxRegisteredNodes * 8>;
    Map series_;
};

}
