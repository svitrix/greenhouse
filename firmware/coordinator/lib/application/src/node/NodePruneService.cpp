#include "NodePruneService.hpp"

namespace gh::app {

void NodePruneService::tick() noexcept {
    const uint32_t now = clock_.nowMs();
    for (const auto& snap : reg_.snapshotAll()) {
        if (!snap.online) continue;
        // last_seen_ms == 0 means the node was registered (presence frame) but
        // never reported a sample. Treat it as infinitely old so the slot is
        // still reclaimable past the TTL — otherwise a node that announces and
        // then goes silent leaks its registry slot forever. With unsigned
        // arithmetic now - 0 == now, so it prunes once uptime exceeds the TTL.
        if (now - snap.last_seen_ms > offline_threshold_ms_) {
            reg_.markOffline(snap.id);
        }
    }
}

}
