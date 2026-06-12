#ifdef ARDUINO

#include "RestNodeAliasRoutes.hpp"
#include "RestHelpers.hpp"
#include "entities/NodeId.hpp"
#include <ArduinoJson.h>

namespace gh::presentation {

void RestNodeAliasRoutes::registerOn(AsyncWebServer& server) noexcept {
    server.on("^/api/nodes/([0-9A-Fa-f]{16})/alias$", HTTP_PUT,
        [](AsyncWebServerRequest*) {},
        nullptr,
        [this](AsyncWebServerRequest* req, uint8_t* data, size_t len,
                size_t index, size_t total) {
            const auto id = gh::domain::NodeId::parseHex16(
                req->pathArg(0).c_str());
            if (!id) {
                rest::sendError(req, 400, "bad_ieee", "invalid IEEE address");
                return;
            }
            if (!reg_.snapshot(*id)) {
                rest::sendError(req, 404, "unknown_node", "no such node");
                return;
            }
            size_t body_len = 0;
            if (!rest::collectBody(req, data, len, index, total,
                                   body_buf_, sizeof(body_buf_), body_len)) {
                return;
            }
            JsonDocument body;
            if (deserializeJson(body, body_buf_, body_len)) {
                rest::sendError(req, 400, "bad_json", "malformed JSON body");
                return;
            }
            const char* alias = body["alias"] | "";
            const auto rc = aliases_.setAlias(*id, alias);
            if (rc == gh::domain::ErrorCode::AliasTooLong) {
                rest::sendError(req, 400, "alias_too_long", "alias exceeds limit");
                return;
            }
            if (rc != gh::domain::ErrorCode::Ok) {
                rest::sendError(req, 500, "store_failed", "failed to persist alias");
                return;
            }
            auto* resp = req->beginResponseStream("application/json");
            resp->addHeader("Cache-Control", "no-store");
            JsonDocument out;
            out["ok"]    = true;
            out["alias"] = alias;
            serializeJson(out, *resp);
            req->send(resp);
        });
}

}  // namespace gh::presentation

#endif  // ARDUINO
