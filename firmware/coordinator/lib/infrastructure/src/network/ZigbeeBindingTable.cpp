#include "ZigbeeBindingTable.hpp"

namespace gh::infra {

void ZigbeeBindingTable::onDeviceAnnounced(uint16_t short_addr, uint64_t ieee) noexcept {
    for (auto it = map_.begin(); it != map_.end();) {
        if (it->second.ieee == ieee) it = map_.erase(it);
        else ++it;
    }
    if (map_.full()) {
        map_.erase(map_.begin());
    }
    map_[short_addr] = gh::domain::NodeId{ieee};
}

void ZigbeeBindingTable::onDeviceLeft(uint16_t short_addr) noexcept {
    map_.erase(short_addr);
}

std::optional<gh::domain::NodeId>
ZigbeeBindingTable::resolve(uint16_t short_addr) const noexcept {
    auto it = map_.find(short_addr);
    if (it == map_.end()) return std::nullopt;
    return it->second;
}

std::optional<uint16_t>
ZigbeeBindingTable::shortAddrFor(gh::domain::NodeId id) const noexcept {
    for (const auto& entry : map_) {
        if (entry.second == id) return entry.first;
    }
    return std::nullopt;
}

}
