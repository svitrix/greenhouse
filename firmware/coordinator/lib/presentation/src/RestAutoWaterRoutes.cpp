#ifdef ARDUINO

#include "RestAutoWaterRoutes.hpp"
#include "AutoWaterView.hpp"
#include <ArduinoJson.h>

namespace gh::presentation {

void RestAutoWaterRoutes::registerOn(AsyncWebServer& server) noexcept {
    server.on("/api/auto-water/state", HTTP_GET,
        [this](AsyncWebServerRequest* req) {
            auto* resp = req->beginResponseStream("application/json");
            resp->addHeader("Cache-Control", "no-store");
            JsonDocument doc;
            AutoWaterView::build(irrigation_.lastDecision(),
                                 doc.to<JsonObject>());
            serializeJson(doc, *resp);
            req->send(resp);
        });
}

}  // namespace gh::presentation

#endif  // ARDUINO
