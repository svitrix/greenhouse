#pragma once
#include "ports/INodeRegistry.hpp"

namespace gh::test {

class FakeNodeRegistry final : public gh::domain::INodeRegistry {
public:
    etl::vector<gh::domain::NodeSnapshot,
                gh::domain::kMaxRegisteredNodes> snapshots;

    [[nodiscard]] gh::domain::ErrorCode recordPresence(
        gh::domain::NodeId, uint16_t, uint32_t, uint16_t) noexcept override {
        return gh::domain::ErrorCode::Ok;
    }
    void recordSample(gh::domain::NodeId, gh::domain::ChannelSample) noexcept override {}
    void recordRssi  (gh::domain::NodeId, int8_t)                     noexcept override {}
    void markOffline (gh::domain::NodeId)                             noexcept override {}
    void forget      (gh::domain::NodeId)                             noexcept override {}

    [[nodiscard]] std::optional<gh::domain::NodeSnapshot>
        snapshot(gh::domain::NodeId id) const noexcept override {
        for (const auto& s : snapshots) if (s.id == id) return s;
        return std::nullopt;
    }
    [[nodiscard]] etl::vector<gh::domain::NodeSnapshot, gh::domain::kMaxRegisteredNodes>
        snapshotAll() const noexcept override { return snapshots; }
};

}
