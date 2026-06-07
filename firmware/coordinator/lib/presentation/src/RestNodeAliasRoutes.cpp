#ifdef ARDUINO

#include "RestNodeAliasRoutes.hpp"
#include "JsonHelpers.hpp"
#include <ArduinoJson.h>

namespace gh::presentation {

void RestNodeAliasRoutes::registerOn(AsyncWebServer& server) noexcept {
    server.on("^/api/nodes/([0-9A-Fa-f]{16})/alias$", HTTP_PUT,
        [this](AsyncWebServerRequest*) {},
        nullptr,
        [this](AsyncWebServerRequest* req, uint8_t* data, size_t len,
                size_t, size_t) {
            const auto id = parseIeeeFromPath(req->url().c_str());
            if (!id) {
                req->send(400, "application/json", "{\"error\":\"bad_ieee\"}");
                return;
            }
            if (!reg_.snapshot(*id)) {
                req->send(404, "application/json", "{\"error\":\"unknown_node\"}");
                return;
            }
            JsonDocument body;
            if (deserializeJson(body, reinterpret_cast<const char*>(data), len)) {
                req->send(400, "application/json", "{\"error\":\"bad_json\"}");
                return;
            }
            const char* alias = body["alias"] | "";
            const auto rc = aliases_.setAlias(*id, alias);
            if (rc == gh::domain::ErrorCode::AliasTooLong) {
                req->send(400, "application/json", "{\"error\":\"alias_too_long\"}");
                return;
            }
            if (rc != gh::domain::ErrorCode::Ok) {
                req->send(500, "application/json", "{\"error\":\"store_failed\"}");
                return;
            }
            JsonDocument out;
            out["ok"]    = true;
            out["alias"] = alias;
            auto* resp = req->beginResponseStream("application/json");
            serializeJson(out, *resp);
            req->send(resp);
        }).addMiddleware(&auth_);
}

}  // namespace gh::presentation

#endif  // ARDUINO
