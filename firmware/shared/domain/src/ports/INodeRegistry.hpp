#pragma once
#include <cstdint>
#include <optional>
#include <etl/vector.h>
#include "entities/NodeSnapshot.hpp"
#include "errors/ErrorCode.hpp"

namespace gh::domain {

inline constexpr size_t kMaxRegisteredNodes = 8;

class INodeRegistry {
public:
    virtual ~INodeRegistry() = default;

    // Returns ErrorCode::BoundedStorageExceeded when capacity is full of
    // online nodes (no offline slot to evict).
    [[nodiscard]] virtual ErrorCode recordPresence(
        NodeId, uint16_t short_addr, uint32_t mask, uint16_t proto_version) noexcept = 0;

    virtual void recordSample(NodeId, ChannelSample) noexcept = 0;
    virtual void recordRssi  (NodeId, int8_t dbm)    noexcept = 0;
    virtual void markOffline (NodeId)                noexcept = 0;
    virtual void forget      (NodeId)                noexcept = 0;

    [[nodiscard]] virtual std::optional<NodeSnapshot> snapshot(NodeId) const noexcept = 0;
    [[nodiscard]] virtual etl::vector<NodeSnapshot, kMaxRegisteredNodes>
        snapshotAll() const noexcept = 0;
};

}
