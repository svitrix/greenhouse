#include "NodePruneService.hpp"

namespace gh::app {

void NodePruneService::tick() noexcept {
    const uint32_t now = clock_.nowMs();
    for (const auto& snap : reg_.snapshotAll()) {
        if (!snap.online) continue;
        if (snap.last_seen_ms == 0) continue;
        if (now - snap.last_seen_ms > offline_threshold_ms_) {
            reg_.markOffline(snap.id);
        }
    }
}

}
