#pragma once
#include <etl/flat_set.h>
#include "Quantity.hpp"
#include "entities/NodeId.hpp"
#include "entities/SensorKind.hpp"
#include "ports/IMqttClient.hpp"
#include "ports/INodeAliasStore.hpp"
#include "ports/INodeRegistry.hpp"

namespace gh::presentation {

// Reconciles per-node HA Discovery entity configs against the registry's
// present_mask. Publishes one HA "device" per Zigbee node; one HA "sensor"
// entity per (channel_id, quantity) row in kChannelAttrTable.
//
// Topic scheme (retained):
//   homeassistant/sensor/gh_node_<ieee16hex>_<kind>_<quantity>/config
//
// `reconcile()` is idempotent — calling it on every tick republishes only
// entries that newly appeared, and emits an empty retained payload for
// entries that disappeared (HA's standard "unpublish" pattern).
class HomeAssistantDiscoveryService {
public:
    HomeAssistantDiscoveryService(gh::domain::IMqttClient&     mqtt,
                                    gh::domain::INodeRegistry&   reg,
                                    gh::domain::INodeAliasStore& aliases,
                                    const char*                  device_id) noexcept
        : mqtt_{mqtt}, reg_{reg}, aliases_{aliases}, device_id_{device_id} {}

    void reconcile() noexcept;

private:
    gh::domain::IMqttClient&     mqtt_;
    gh::domain::INodeRegistry&   reg_;
    gh::domain::INodeAliasStore& aliases_;
    const char*                  device_id_;

    struct Key {
        gh::domain::NodeId     id;
        gh::protocol::Quantity q;
        [[nodiscard]] bool operator<(const Key& o) const noexcept {
            if (id.ieee != o.id.ieee) return id.ieee < o.id.ieee;
            return static_cast<int>(q) < static_cast<int>(o.q);
        }
    };
    // Capacity: kMaxRegisteredNodes (8) * kMaxChannelSamplesPerNode (8) = 64.
    etl::flat_set<Key, 64> published_;

    void publishEntity_  (gh::domain::NodeId, gh::protocol::Quantity,
                          gh::domain::SensorKind) noexcept;
    void unpublishEntity_(gh::domain::NodeId, gh::protocol::Quantity) noexcept;
};

}
