#include "InMemoryHistoryStore.hpp"

namespace gh::infra {

void InMemoryHistoryStore::recordPoint(
    gh::domain::NodeId node, gh::domain::SensorKind kind,
    gh::protocol::Quantity q, Point p) noexcept
{
    LockGuard lock{mutex_};
    const Key key{node, kind, q};
    auto it = series_.find(key);
    if (it == series_.end()) {
        if (series_.full()) return;
        series_.insert({key, Series{}});
        it = series_.find(key);
    }
    auto& buf = it->second;
    const uint32_t cutoff =
        (p.monotonic_ms > kWindowMs) ? p.monotonic_ms - kWindowMs : 0u;
    while (!buf.empty() && buf.front().monotonic_ms < cutoff) {
        buf.pop();
    }
    if (buf.full()) buf.pop();
    buf.push(p);
}

void InMemoryHistoryStore::forgetNode(gh::domain::NodeId node) noexcept {
    LockGuard lock{mutex_};
    for (auto it = series_.begin(); it != series_.end();) {
        if (it->first.node == node) it = series_.erase(it);
        else ++it;
    }
}

etl::vector<InMemoryHistoryStore::Point, gh::domain::kHistoryMaxPointsPerSeries>
InMemoryHistoryStore::query(
    gh::domain::NodeId node, gh::domain::SensorKind kind,
    gh::protocol::Quantity q, uint32_t since) const noexcept
{
    LockGuard lock{mutex_};
    etl::vector<Point, gh::domain::kHistoryMaxPointsPerSeries> out;
    auto it = series_.find(Key{node, kind, q});
    if (it == series_.end()) return out;
    for (const auto& p : it->second) {
        if (p.monotonic_ms >= since) out.push_back(p);
    }
    return out;
}

}
