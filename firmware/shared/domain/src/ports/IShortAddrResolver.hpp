#pragma once
#include <cstdint>
#include <optional>
#include "entities/NodeId.hpp"

namespace gh::domain {

// The short_addr ↔ IEEE (NodeId) binding directory the application sees.
// Implemented by the infra ZigbeeBindingTable. Keeping the application
// dependent on this interface (not the concrete infra type) preserves the
// inward-only dependency rule (DIP). The router mutates the binding on ZDO
// announce/leave and reads it (resolve) on every report to cross-check the
// APS source IEEE.
class IShortAddrResolver {
public:
    virtual ~IShortAddrResolver() = default;

    // ZDO Device_Announce (or adapter self-heal seed): bind short_addr → ieee.
    virtual void onDeviceAnnounced(uint16_t short_addr, uint64_t ieee) noexcept = 0;

    // ZDO Device_Leave confirmed: release the binding for short_addr.
    virtual void onDeviceLeft(uint16_t short_addr) noexcept = 0;

    // The NodeId currently bound to short_addr, or nullopt if unknown.
    [[nodiscard]] virtual std::optional<NodeId>
        resolve(uint16_t short_addr) const noexcept = 0;
};

}
