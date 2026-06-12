#ifdef ARDUINO

#include "RestNodesRoutes.hpp"
#include "RestHelpers.hpp"
#include "NodeViewBuilder.hpp"
#include "entities/NodeId.hpp"
#include <ArduinoJson.h>

namespace gh::presentation {

void RestNodesRoutes::registerOn(AsyncWebServer& server) noexcept {
    server.on("/api/nodes", HTTP_GET,
        [this](AsyncWebServerRequest* req) {
            auto* resp = req->beginResponseStream("application/json");
            resp->addHeader("Cache-Control", "no-store");
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
        });

    server.on("^/api/nodes/([0-9A-Fa-f]{16})$", HTTP_GET,
        [this](AsyncWebServerRequest* req) {
            const auto id = gh::domain::NodeId::parseHex16(
                req->pathArg(0).c_str());
            if (!id) {
                rest::sendError(req, 400, "bad_ieee", "invalid IEEE address");
                return;
            }
            auto snap = reg_.snapshot(*id);
            if (!snap) {
                rest::sendError(req, 404, "unknown_node", "no such node");
                return;
            }
            auto* resp = req->beginResponseStream("application/json");
            resp->addHeader("Cache-Control", "no-store");
            JsonDocument doc;
            NodeViewBuilder::build(*snap, aliases_, clock_.nowMs(),
                                   doc.to<JsonObject>());
            serializeJson(doc, *resp);
            req->send(resp);
        });
}

}  // namespace gh::presentation

#endif  // ARDUINO
