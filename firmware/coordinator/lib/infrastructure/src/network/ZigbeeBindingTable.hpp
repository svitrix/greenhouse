#pragma once
#include <cstdint>
#include <optional>
#include <etl/flat_map.h>
#include "entities/NodeId.hpp"
#include "ports/IShortAddrResolver.hpp"

namespace gh::infra {

class ZigbeeBindingTable final : public gh::domain::IShortAddrResolver {
public:
    void onDeviceAnnounced(uint16_t short_addr, uint64_t ieee) noexcept override;
    void onDeviceLeft     (uint16_t short_addr)                noexcept override;
    [[nodiscard]] std::optional<gh::domain::NodeId>
        resolve(uint16_t short_addr) const noexcept override;
    // Reverse lookup over the <=8 live entries (O(n), n<=8). Avoids scanning
    // the full 0..0xFFFE short-address space on node removal.
    [[nodiscard]] std::optional<uint16_t>
        shortAddrFor(gh::domain::NodeId id) const noexcept;
private:
    etl::flat_map<uint16_t, gh::domain::NodeId, 8> map_;
};

}
