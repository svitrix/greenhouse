#ifdef ARDUINO

#include "RestAutoWaterRoutes.hpp"
#include "irrigation/AutoWaterDecision.hpp"
#include <ArduinoJson.h>

namespace gh::presentation {

void RestAutoWaterRoutes::registerOn(AsyncWebServer& server) noexcept {
    server.on("/api/auto-water/state", HTTP_GET,
        [this](AsyncWebServerRequest* req) {
            const auto& d = irrigation_.lastDecision();

            auto* resp = req->beginResponseStream("application/json");
            JsonDocument doc;
            if (d.avg_moisture_pct) {
                doc["avg_moisture_pct"] = *d.avg_moisture_pct;
            } else {
                doc["avg_moisture_pct"] = nullptr;
            }

            JsonArray fresh = doc["fresh_sources"].to<JsonArray>();
            for (const auto& id : d.fresh_sources) {
                const auto hex = id.toHex16();
                fresh.add(JsonString(hex.data()));
            }

            JsonArray stale = doc["stale_sources"].to<JsonArray>();
            for (const auto& id : d.stale_sources) {
                const auto hex = id.toHex16();
                stale.add(JsonString(hex.data()));
            }

            doc["last_decision_ms"] = d.monotonic_ms;
            doc["last_decision"]    = gh::app::outcomeCode(d.outcome);

            serializeJson(doc, *resp);
            req->send(resp);
        }).addMiddleware(&auth_);
}

}  // namespace gh::presentation

#endif  // ARDUINO
