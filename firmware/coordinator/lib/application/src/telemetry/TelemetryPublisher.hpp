#pragma once
#include <cstdint>
#include <etl/flat_map.h>
#include "Quantity.hpp"
#include "entities/NodeId.hpp"
#include "ports/IClock.hpp"
#include "ports/IMqttClient.hpp"
#include "ports/INodeRegistry.hpp"

namespace gh::app {

// Per-node MQTT telemetry publisher.
//
// Topic scheme:
//   greenhouse/<device_id>/nodes/<ieee16hex>/online        (retain, change-only)
//   greenhouse/<device_id>/nodes/<ieee16hex>/present_mask  (retain, change-only)
//   greenhouse/<device_id>/nodes/<ieee16hex>/proto_version (retain, change-only)
//   greenhouse/<device_id>/nodes/<ieee16hex>/rssi_dbm      (non-retain, every tick)
//   greenhouse/<device_id>/nodes/<ieee16hex>/<quantity>    (non-retain, change-only)
//
// Tick-driven. No heap; bounded per-node and per-(node,quantity) caches.
// Disconnect is a no-op (caller must handle reconnect; we will rebroadcast on
// next change after isConnected() flips back).
class TelemetryPublisher {
public:
    TelemetryPublisher(gh::domain::IMqttClient&   mqtt,
                         gh::domain::INodeRegistry& reg,
                         gh::domain::IClock&        clock,
                         const char*                device_id) noexcept
        : mqtt_{mqtt}, reg_{reg}, clock_{clock}, device_id_{device_id} {}

    void tick() noexcept;

private:
    gh::domain::IMqttClient&   mqtt_;
    gh::domain::INodeRegistry& reg_;
    gh::domain::IClock&        clock_;
    const char*                device_id_;

    struct ValueKey {
        gh::domain::NodeId     id;
        gh::protocol::Quantity q;
        [[nodiscard]] bool operator<(const ValueKey& o) const noexcept {
            if (id.ieee != o.id.ieee) return id.ieee < o.id.ieee;
            return static_cast<int>(q) < static_cast<int>(o.q);
        }
    };

    // Capacity: kMaxRegisteredNodes (8) * kMaxChannelSamplesPerNode (8) = 64.
    etl::flat_map<ValueKey, float,    64> last_values_;
    etl::flat_map<gh::domain::NodeId, bool,     gh::domain::kMaxRegisteredNodes> last_online_;
    etl::flat_map<gh::domain::NodeId, uint32_t, gh::domain::kMaxRegisteredNodes> last_mask_;
    etl::flat_map<gh::domain::NodeId, uint16_t, gh::domain::kMaxRegisteredNodes> last_proto_;
};

}
