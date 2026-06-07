#pragma once
#include <array>
#include <cstdint>

namespace gh::infra {

// Persists per-coordinator randomised network identifiers in NVS
// namespace "zigbee_net". On first boot generates a fresh 8-byte
// ExtPanId (never all-zero, never all-FF) and stores it. Subsequent
// boots load from NVS.
class NvsZigbeeNetStore {
public:
    NvsZigbeeNetStore() noexcept = default;

    // Returns the persisted ExtPanId, generating + storing one if absent.
    [[nodiscard]] std::array<uint8_t, 8> loadOrGenerate() noexcept;
};

}  // namespace gh::infra
