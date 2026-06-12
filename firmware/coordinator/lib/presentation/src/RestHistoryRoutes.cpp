#ifdef ARDUINO

#include "RestHistoryRoutes.hpp"
#include "RestHelpers.hpp"
#include "JsonHelpers.hpp"
#include <ArduinoJson.h>

namespace gh::presentation {

namespace {
constexpr uint32_t kHistoryMinHours    = 1;
constexpr uint32_t kHistoryMaxHours    = 24;
constexpr uint32_t kMsPerHour          = 60u * 60u * 1000u;
}  // namespace

void RestHistoryRoutes::registerOn(AsyncWebServer& server) noexcept {
    server.on("/api/history", HTTP_GET,
        [this](AsyncWebServerRequest* req) {
            if (!req->hasParam("ieee") || !req->hasParam("kind") ||
                !req->hasParam("quantity") || !req->hasParam("hours")) {
                rest::sendError(req, 400, "missing_query", "ieee/kind/quantity/hours required");
                return;
            }
            const auto id    = gh::domain::NodeId::parseHex16(
                req->getParam("ieee")->value().c_str());
            const auto kind  = kindFromCode(req->getParam("kind")->value().c_str());
            const auto qty   = quantityFromCode(req->getParam("quantity")->value().c_str());
            const auto hours = static_cast<uint32_t>(
                req->getParam("hours")->value().toInt());
            if (!id || !kind || !qty ||
                hours < kHistoryMinHours || hours > kHistoryMaxHours) {
                rest::sendError(req, 400, "bad_query", "invalid query parameters");
                return;
            }
            const uint32_t since = clock_.nowMs() - hours * kMsPerHour;
            const auto pts = hist_.query(*id, *kind, *qty, since);

            // Variable-length series — stream it instead of one large stack buf.
            auto* resp = req->beginResponseStream("application/json");
            resp->addHeader("Cache-Control", "no-store");
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
        });
}

}  // namespace gh::presentation

#endif  // ARDUINO
