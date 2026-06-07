#ifdef ARDUINO

#include "RestNodeDeleteRoutes.hpp"
#include "JsonHelpers.hpp"
#include <ArduinoJson.h>

namespace gh::presentation {

void RestNodeDeleteRoutes::registerOn(AsyncWebServer& server) noexcept {
    server.on("^/api/nodes/([0-9A-Fa-f]{16})$", HTTP_DELETE,
        [this](AsyncWebServerRequest* req) {
            const auto id = parseIeeeFromPath(req->url().c_str());
            if (!id) {
                req->send(400, "application/json", "{\"error\":\"bad_ieee\"}");
                return;
            }
            const auto snap = reg_.snapshot(*id);
            if (!snap) {
                req->send(404, "application/json", "{\"error\":\"unknown_node\"}");
                return;
            }
            const auto leave_rc = zb_.requestLeave(*id);
            (void)aliases_.clearAlias(*id);
            hist_.forgetNode(*id);
            reg_.forget(*id);

            JsonDocument out;
            out["ok"]          = true;
            out["leave_acked"] = (leave_rc == gh::domain::ErrorCode::Ok);
            auto* resp = req->beginResponseStream("application/json");
            serializeJson(out, *resp);
            req->send(resp);
        }).addMiddleware(&auth_);
}

}  // namespace gh::presentation

#endif  // ARDUINO
