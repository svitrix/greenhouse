#pragma once
#include <ArduinoJson.h>
#include "irrigation/AutoWaterDecision.hpp"

namespace gh::presentation {

// Serialises an AutoWaterDecision into the shared auto-water view object
// { avg_moisture_pct, fresh_sources[], stale_sources[], last_decision_ms,
//   last_decision }. Built identically by GET /api/auto-water/state and the
// "auto" block of the dashboard payload — this is the single owner of that
// shape. Pure (ArduinoJson + application only), unit-testable under native.
class AutoWaterView {
public:
    static void build(const gh::app::AutoWaterDecision& decision,
                      JsonObject out) noexcept;
};

}  // namespace gh::presentation
