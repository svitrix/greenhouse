#include "DashboardViewBuilder.hpp"
#include "NodeViewBuilder.hpp"
#include "PumpStateView.hpp"
#include "AutoWaterView.hpp"

namespace gh::presentation {

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

    AutoWaterView::build(d, out["auto"].to<JsonObject>());
}

}  // namespace gh::presentation
