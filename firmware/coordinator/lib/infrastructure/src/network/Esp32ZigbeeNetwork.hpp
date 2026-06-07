#pragma once
#include "ports/IZigbeeCoordinator.hpp"
#include "ports/IZigbeeNetwork.hpp"
#include "ZigbeeBindingTable.hpp"

namespace gh::infra {

// Adapter that exposes the IZigbeeNetwork port to application/presentation
// services. Delegates to IZigbeeCoordinator for the on-air commands and
// uses ZigbeeBindingTable to resolve NodeId (IEEE) → short_addr.
class Esp32ZigbeeNetwork final : public gh::domain::IZigbeeNetwork {
public:
    Esp32ZigbeeNetwork(gh::domain::IZigbeeCoordinator& zb,
                       ZigbeeBindingTable& binding) noexcept
        : zb_{zb}, binding_{binding} {}

    [[nodiscard]] gh::domain::ErrorCode permitJoin  (uint8_t duration_s) noexcept override;
    [[nodiscard]] gh::domain::ErrorCode requestLeave(gh::domain::NodeId) noexcept override;

private:
    gh::domain::IZigbeeCoordinator& zb_;
    ZigbeeBindingTable&             binding_;
};

}  // namespace gh::infra
