#include "AutoWaterView.hpp"

namespace gh::presentation {

void AutoWaterView::build(const gh::app::AutoWaterDecision& d,
                          JsonObject out) noexcept {
    if (d.avg_moisture_pct) {
        out["avg_moisture_pct"] = *d.avg_moisture_pct;
    } else {
        out["avg_moisture_pct"] = nullptr;
    }

    JsonArray fresh = out["fresh_sources"].to<JsonArray>();
    for (const auto& id : d.fresh_sources) {
        const auto hex = id.toHex16();
        fresh.add(JsonString(hex.data()));  // JsonString copies into the pool
    }

    JsonArray stale = out["stale_sources"].to<JsonArray>();
    for (const auto& id : d.stale_sources) {
        const auto hex = id.toHex16();
        stale.add(JsonString(hex.data()));
    }

    out["last_decision_ms"] = d.monotonic_ms;
    out["last_decision"]    = gh::app::outcomeCode(d.outcome);
}

}  // namespace gh::presentation
