#include "TelemetryPublisher.hpp"
#include <cmath>
#include <cstdio>

namespace gh::app {

void TelemetryPublisher::tick() noexcept {
    if (!mqtt_.isConnected()) return;

    (void)clock_;  // reserved for future last-seen / staleness annotations

    for (const auto& snap : reg_.snapshotAll()) {
        const auto hex = snap.id.toHex16();

        // online (retained, change-only)
        auto it_on = last_online_.find(snap.id);
        if (it_on == last_online_.end() || it_on->second != snap.online) {
            char topic[96];
            std::snprintf(topic, sizeof(topic),
                "greenhouse/%s/nodes/%s/online", device_id_, hex.data());
            (void)mqtt_.publish(topic, snap.online ? "true" : "false", /*retain*/ true);
            last_online_[snap.id] = snap.online;
        }

        // present_mask (retained, change-only)
        auto it_m = last_mask_.find(snap.id);
        if (it_m == last_mask_.end() || it_m->second != snap.present_mask) {
            char topic[96];
            char payload[8];
            std::snprintf(topic, sizeof(topic),
                "greenhouse/%s/nodes/%s/present_mask", device_id_, hex.data());
            std::snprintf(payload, sizeof(payload),
                "0x%02X", static_cast<unsigned>(snap.present_mask & 0xFFu));
            (void)mqtt_.publish(topic, payload, /*retain*/ true);
            last_mask_[snap.id] = snap.present_mask;
        }

        // proto_version (retained, change-only)
        auto it_p = last_proto_.find(snap.id);
        if (it_p == last_proto_.end() || it_p->second != snap.proto_version) {
            char topic[96];
            char payload[8];
            std::snprintf(topic, sizeof(topic),
                "greenhouse/%s/nodes/%s/proto_version", device_id_, hex.data());
            std::snprintf(payload, sizeof(payload), "%u",
                static_cast<unsigned>(snap.proto_version));
            (void)mqtt_.publish(topic, payload, /*retain*/ true);
            last_proto_[snap.id] = snap.proto_version;
        }

        // rssi (non-retained, every tick)
        {
            char topic[96];
            char payload[8];
            std::snprintf(topic, sizeof(topic),
                "greenhouse/%s/nodes/%s/rssi_dbm", device_id_, hex.data());
            std::snprintf(payload, sizeof(payload), "%d",
                static_cast<int>(snap.last_rssi_dbm));
            (void)mqtt_.publish(topic, payload, /*retain*/ false);
        }

        // Per-quantity samples (non-retained, change-only by 0.01 threshold)
        for (const auto& s : snap.samples) {
            ValueKey k{snap.id, s.quantity};
            auto vit = last_values_.find(k);
            if (vit != last_values_.end() &&
                std::fabs(vit->second - s.value_si) < 0.01f) {
                continue;
            }
            char topic[96];
            char payload[16];
            std::snprintf(topic, sizeof(topic),
                "greenhouse/%s/nodes/%s/%s", device_id_, hex.data(),
                gh::protocol::quantityCode(s.quantity));
            std::snprintf(payload, sizeof(payload), "%.2f",
                static_cast<double>(s.value_si));
            (void)mqtt_.publish(topic, payload, /*retain*/ false);
            last_values_[k] = s.value_si;
        }
    }
}

}
