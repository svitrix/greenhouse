#include "Esp32ZigbeeNetwork.hpp"

namespace gh::infra {

gh::domain::ErrorCode Esp32ZigbeeNetwork::permitJoin(uint8_t duration_s) noexcept {
    if (duration_s < 1 || duration_s > 254) {
        return gh::domain::ErrorCode::SensorOutOfRange;
    }
    return zb_.openPermitJoin(static_cast<uint16_t>(duration_s));
}

gh::domain::ErrorCode Esp32ZigbeeNetwork::requestLeave(gh::domain::NodeId id) noexcept {
    // Walk the binding table to find the short_addr currently bound to this
    // IEEE address. If the device has already left or never joined we still
    // attempt the leave with short_addr = 0xFFFF so the coordinator surfaces
    // a Timeout up to the caller for cleanup.
    const auto bound = binding_.shortAddrFor(id);
    const uint16_t short_addr = bound ? *bound : 0xFFFF;
    return zb_.requestLeave(short_addr, id.ieee);
}

}  // namespace gh::infra
