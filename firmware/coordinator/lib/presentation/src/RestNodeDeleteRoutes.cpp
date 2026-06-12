#ifdef ARDUINO

#include "RestNodeDeleteRoutes.hpp"
#include "RestHelpers.hpp"
#include "entities/NodeId.hpp"
#include <ArduinoJson.h>

namespace gh::presentation {

void RestNodeDeleteRoutes::registerOn(AsyncWebServer& server) noexcept {
    server.on("^/api/nodes/([0-9A-Fa-f]{16})$", HTTP_DELETE,
        [this](AsyncWebServerRequest* req) {
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
            const auto leave_rc = zb_.requestLeave(*id);
            (void)aliases_.clearAlias(*id);
            hist_.forgetNode(*id);
            reg_.forget(*id);

            auto* resp = req->beginResponseStream("application/json");
            resp->addHeader("Cache-Control", "no-store");
            JsonDocument out;
            out["ok"]          = true;
            out["leave_acked"] = (leave_rc == gh::domain::ErrorCode::Ok);
            serializeJson(out, *resp);
            req->send(resp);
        });
}

}  // namespace gh::presentation

#endif  // ARDUINO
