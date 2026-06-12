#include "InMemoryNodeRegistry.hpp"

namespace gh::infra {

gh::domain::ErrorCode InMemoryNodeRegistry::recordPresence(
    gh::domain::NodeId id, uint16_t short_addr,
    uint32_t mask, uint16_t proto_version) noexcept
{
    LockGuard lock{mutex_};
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
    LockGuard lock{mutex_};
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
    LockGuard lock{mutex_};
    auto it = nodes_.find(id);
    if (it == nodes_.end()) return;
    it->second.last_rssi_dbm = dbm;
}

void InMemoryNodeRegistry::markOffline(gh::domain::NodeId id) noexcept {
    LockGuard lock{mutex_};
    auto it = nodes_.find(id);
    if (it != nodes_.end()) it->second.online = false;
}

void InMemoryNodeRegistry::forget(gh::domain::NodeId id) noexcept {
    LockGuard lock{mutex_};
    nodes_.erase(id);
    if (history_ != nullptr) history_->forgetNode(id);
}

std::optional<gh::domain::NodeSnapshot>
InMemoryNodeRegistry::snapshot(gh::domain::NodeId id) const noexcept {
    LockGuard lock{mutex_};
    auto it = nodes_.find(id);
    if (it == nodes_.end()) return std::nullopt;
    return it->second;
}

etl::vector<gh::domain::NodeSnapshot, gh::domain::kMaxRegisteredNodes>
InMemoryNodeRegistry::snapshotAll() const noexcept {
    LockGuard lock{mutex_};
    etl::vector<gh::domain::NodeSnapshot, gh::domain::kMaxRegisteredNodes> out;
    for (const auto& kv : nodes_) out.push_back(kv.second);
    return out;
}

bool InMemoryNodeRegistry::evictOneOffline_() noexcept {
    auto victim = nodes_.end();
    for (auto it = nodes_.begin(); it != nodes_.end(); ++it) {
        if (it->second.online) continue;
        if (victim == nodes_.end() ||
            it->second.last_seen_ms < victim->second.last_seen_ms) {
            victim = it;
        }
    }
    if (victim == nodes_.end()) return false;
    const gh::domain::NodeId evicted = victim->first;
    nodes_.erase(victim);
    if (history_ != nullptr) history_->forgetNode(evicted);
    return true;
}

}
