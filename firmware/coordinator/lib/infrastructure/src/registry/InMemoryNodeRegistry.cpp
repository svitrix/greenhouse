#include "InMemoryNodeRegistry.hpp"

namespace gh::infra {

gh::domain::ErrorCode InMemoryNodeRegistry::recordPresence(
    gh::domain::NodeId id, uint16_t short_addr,
    uint32_t mask, uint16_t proto_version) noexcept
{
    auto it = nodes_.find(id);
    if (it == nodes_.end()) {
        if (nodes_.full() && !evictOneOffline_()) {
            return gh::domain::ErrorCode::BoundedStorageExceeded;
        }
        gh::domain::NodeSnapshot snap;
        snap.id                     = id;
        snap.short_addr             = short_addr;
        snap.present_mask           = mask;
        snap.proto_version          = proto_version;
        snap.online                 = true;
        nodes_.insert({id, snap});
        return gh::domain::ErrorCode::Ok;
    }
    it->second.short_addr    = short_addr;
    it->second.present_mask  = mask;
    it->second.proto_version = proto_version;
    it->second.online        = true;
    return gh::domain::ErrorCode::Ok;
}

void InMemoryNodeRegistry::recordSample(
    gh::domain::NodeId id, gh::domain::ChannelSample s) noexcept
{
    auto it = nodes_.find(id);
    if (it == nodes_.end()) return;
    auto& snap = it->second;
    snap.last_seen_ms = s.monotonic_ms;
    snap.online       = true;
    for (auto& existing : snap.samples) {
        if (existing.kind == s.kind && existing.quantity == s.quantity) {
            existing = s;
            return;
        }
    }
    if (!snap.samples.full()) {
        snap.samples.push_back(s);
    }
}

void InMemoryNodeRegistry::recordRssi(gh::domain::NodeId id, int8_t dbm) noexcept {
    auto it = nodes_.find(id);
    if (it == nodes_.end()) return;
    it->second.last_rssi_dbm = dbm;
}

void InMemoryNodeRegistry::markOffline(gh::domain::NodeId id) noexcept {
    auto it = nodes_.find(id);
    if (it != nodes_.end()) it->second.online = false;
}

void InMemoryNodeRegistry::forget(gh::domain::NodeId id) noexcept {
    nodes_.erase(id);
}

std::optional<gh::domain::NodeSnapshot>
InMemoryNodeRegistry::snapshot(gh::domain::NodeId id) const noexcept {
    auto it = nodes_.find(id);
    if (it == nodes_.end()) return std::nullopt;
    return it->second;
}

etl::vector<gh::domain::NodeSnapshot, gh::domain::kMaxRegisteredNodes>
InMemoryNodeRegistry::snapshotAll() const noexcept {
    etl::vector<gh::domain::NodeSnapshot, gh::domain::kMaxRegisteredNodes> out;
    for (const auto& kv : nodes_) out.push_back(kv.second);
    return out;
}

bool InMemoryNodeRegistry::evictOneOffline_() noexcept {
    for (auto it = nodes_.begin(); it != nodes_.end(); ++it) {
        if (!it->second.online) {
            nodes_.erase(it);
            return true;
        }
    }
    return false;
}

}
