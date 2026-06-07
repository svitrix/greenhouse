#ifdef ARDUINO

#include "RestHistoryRoutes.hpp"
#include "JsonHelpers.hpp"
#include <ArduinoJson.h>

namespace gh::presentation {

void RestHistoryRoutes::registerOn(AsyncWebServer& server) noexcept {
    server.on("/api/history", HTTP_GET,
        [this](AsyncWebServerRequest* req) {
            if (!req->hasParam("ieee") || !req->hasParam("kind") ||
                !req->hasParam("quantity") || !req->hasParam("hours")) {
                req->send(400, "application/json", "{\"error\":\"missing_query\"}");
                return;
            }
            const auto id    = gh::domain::NodeId::parseHex16(
                req->getParam("ieee")->value().c_str());
            const auto kind  = kindFromCode(req->getParam("kind")->value().c_str());
            const auto qty   = quantityFromCode(req->getParam("quantity")->value().c_str());
            const auto hours = static_cast<uint32_t>(
                req->getParam("hours")->value().toInt());
            if (!id || !kind || !qty || hours < 1 || hours > 24) {
                req->send(400, "application/json", "{\"error\":\"bad_query\"}");
                return;
            }
            const uint32_t since = clock_.nowMs() - hours * 60u * 60u * 1000u;
            const auto pts = hist_.query(*id, *kind, *qty, since);

            auto* resp = req->beginResponseStream("application/json");
            resp->print("{\"data\":[");
            for (size_t i = 0; i < pts.size(); ++i) {
                if (i > 0) resp->print(',');
                JsonDocument p;
                p["ts_ms"] = pts[i].monotonic_ms;
                p["value"] = pts[i].value;
                serializeJson(p, *resp);
            }
            resp->print("]}");
            req->send(resp);
        }).addMiddleware(&auth_);
}

}  // namespace gh::presentation

#endif  // ARDUINO
