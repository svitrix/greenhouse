#pragma once
#include <ArduinoJson.h>
#include <cstdint>
#include "ports/INodeRegistry.hpp"
#include "ports/INodeAliasStore.hpp"
#include "entities/PumpState.hpp"
#include "irrigation/AutoWaterDecision.hpp"

namespace gh::presentation {

// Builds the combined dashboard payload { ts_ms, nodes[], pump{}, auto{} } —
// the single source the SSE push (/api/events) and the GET /api/dashboard
// fallback both emit. Pure (ArduinoJson + domain only, no ESPAsyncWebServer /
// Arduino), so it compiles and is unit-tested under the native env.
class DashboardViewBuilder {
public:
    static void build(gh::domain::INodeRegistry&         reg,
                      const gh::domain::INodeAliasStore&  aliases,
                      gh::domain::PumpState               pump_state,
                      const gh::app::AutoWaterDecision&   decision,
                      uint32_t                            now_ms,
                      JsonObject                          out) noexcept;
};

}
