#pragma once
#include <etl/flat_map.h>
#include "ports/INodeRegistry.hpp"

namespace gh::infra {

class InMemoryNodeRegistry final : public gh::domain::INodeRegistry {
public:
    [[nodiscard]] gh::domain::ErrorCode recordPresence(
        gh::domain::NodeId, uint16_t short_addr,
        uint32_t mask, uint16_t proto_version) noexcept override;

    void recordSample(gh::domain::NodeId, gh::domain::ChannelSample) noexcept override;
    void recordRssi  (gh::domain::NodeId, int8_t dbm)                 noexcept override;
    void markOffline (gh::domain::NodeId)                             noexcept override;
    void forget      (gh::domain::NodeId)                             noexcept override;

    [[nodiscard]] std::optional<gh::domain::NodeSnapshot>
        snapshot(gh::domain::NodeId) const noexcept override;
    [[nodiscard]] etl::vector<gh::domain::NodeSnapshot, gh::domain::kMaxRegisteredNodes>
        snapshotAll() const noexcept override;

private:
    using Map = etl::flat_map<gh::domain::NodeId, gh::domain::NodeSnapshot,
                              gh::domain::kMaxRegisteredNodes>;
    Map nodes_;

    [[nodiscard]] bool evictOneOffline_() noexcept;
};

}
