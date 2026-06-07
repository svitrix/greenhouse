#include "DashboardViewBuilder.hpp"
#include "NodeViewBuilder.hpp"

namespace gh::presentation {
namespace {

const char* pumpStateCode(gh::domain::PumpState s) noexcept {
    switch (s) {
        case gh::domain::PumpState::Off:          return "OFF";
        case gh::domain::PumpState::On:           return "ON";
        case gh::domain::PumpState::SafetyLocked: return "LOCKED";
    }
    return "OFF";
}

}  // namespace

void DashboardViewBuilder::build(gh::domain::INodeRegistry&        reg,
                                 const gh::domain::INodeAliasStore& aliases,
                                 gh::domain::PumpState              pump_state,
                                 const gh::app::AutoWaterDecision&  d,
                                 uint32_t                           now_ms,
                                 JsonObject                         out) noexcept {
    out["ts_ms"] = now_ms;

    JsonArray nodes = out["nodes"].to<JsonArray>();
    for (const auto& snap : reg.snapshotAll()) {
        JsonObject n = nodes.add<JsonObject>();
        NodeViewBuilder::build(snap, aliases, now_ms, n);
    }

    JsonObject pump = out["pump"].to<JsonObject>();
    pump["state"]       = pumpStateCode(pump_state);
    pump["remaining_s"] = 0;
    pump["last_run_ms"] = 0;

    JsonObject aw = out["auto"].to<JsonObject>();
    if (d.avg_moisture_pct) {
        aw["avg_moisture_pct"] = *d.avg_moisture_pct;
    } else {
        aw["avg_moisture_pct"] = nullptr;
    }
    JsonArray fresh = aw["fresh_sources"].to<JsonArray>();
    for (const auto& id : d.fresh_sources) {
        const auto hex = id.toHex16();
        fresh.add(JsonString(hex.data()));
    }
    JsonArray stale = aw["stale_sources"].to<JsonArray>();
    for (const auto& id : d.stale_sources) {
        const auto hex = id.toHex16();
        stale.add(JsonString(hex.data()));
    }
    aw["last_decision_ms"] = d.monotonic_ms;
    aw["last_decision"]    = gh::app::outcomeCode(d.outcome);
}

}  // namespace gh::presentation
