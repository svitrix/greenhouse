#ifdef ARDUINO

#include "RestNodesRoutes.hpp"
#include "JsonHelpers.hpp"
#include "NodeViewBuilder.hpp"
#include <ArduinoJson.h>

namespace gh::presentation {

void RestNodesRoutes::registerOn(AsyncWebServer& server) noexcept {
    server.on("/api/nodes", HTTP_GET,
        [this](AsyncWebServerRequest* req) {
            auto* resp = req->beginResponseStream("application/json");
            JsonDocument doc;
            doc["ts_ms"] = clock_.nowMs();
            JsonArray nodes = doc["nodes"].to<JsonArray>();
            const uint32_t now = clock_.nowMs();
            for (const auto& snap : reg_.snapshotAll()) {
                JsonObject n = nodes.add<JsonObject>();
                NodeViewBuilder::build(snap, aliases_, now, n);
            }
            serializeJson(doc, *resp);
            req->send(resp);
        })
        .addMiddleware(&auth_);

    server.on("^/api/nodes/([0-9A-Fa-f]{16})$", HTTP_GET,
        [this](AsyncWebServerRequest* req) {
            const auto id = parseIeeeFromPath(req->url().c_str());
            if (!id) {
                req->send(400, "application/json", "{\"error\":\"bad_ieee\"}");
                return;
            }
            auto snap = reg_.snapshot(*id);
            if (!snap) {
                req->send(404, "application/json", "{\"error\":\"unknown_node\"}");
                return;
            }
            auto* resp = req->beginResponseStream("application/json");
            JsonDocument doc;
            JsonObject root = doc.to<JsonObject>();
            NodeViewBuilder::build(*snap, aliases_, clock_.nowMs(), root);
            serializeJson(doc, *resp);
            req->send(resp);
        })
        .addMiddleware(&auth_);
}

}  // namespace gh::presentation

#endif  // ARDUINO
