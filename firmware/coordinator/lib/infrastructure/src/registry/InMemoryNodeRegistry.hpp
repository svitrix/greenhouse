#pragma once
#include <etl/flat_map.h>
#include "concurrency/RecursiveMutex.hpp"
#include "ports/INodeHistoryStore.hpp"
#include "ports/INodeRegistry.hpp"

namespace gh::infra {

// IEEE-keyed live node snapshot store.
//
// Synchronisation model: every public method takes a recursive mutex for
// its whole body, and snapshot readers copy out under the lock. This is
// required because `recordPresence` (Zigbee task) inserts/erases entries
// in the `etl::flat_map`, invalidating iterators, while `snapshot*` /
// `markOffline` run from the telemetry / REST / prune tasks. Without the
// lock a concurrent insert/erase would tear a reader's iteration.
//
// On the native host build the mutex is a no-op (tests are single-threaded).
class InMemoryNodeRegistry final : public gh::domain::INodeRegistry {
public:
    // Optional history sink. When set, evicting a node from the registry
    // also forgets its history series (so eviction does not leak orphan
    // series that fill the bounded history map). Wired in the composition
    // root after both stores exist.
    void setHistoryStore(gh::domain::INodeHistoryStore* history) noexcept {
        history_ = history;
    }

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
    gh::domain::INodeHistoryStore* history_ = nullptr;
    mutable RecursiveMutex mutex_;

    // Evicts the offline node with the oldest `last_seen_ms` (LRU among
    // offline). Forgets its history too. Returns false if no offline node.
    // Caller must already hold the lock.
    [[nodiscard]] bool evictOneOffline_() noexcept;
};

}
