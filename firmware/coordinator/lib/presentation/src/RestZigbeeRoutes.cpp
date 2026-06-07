#ifdef ARDUINO

#include "RestZigbeeRoutes.hpp"
#include <ArduinoJson.h>

namespace gh::presentation {

void RestZigbeeRoutes::registerOn(AsyncWebServer& server) noexcept {
    server.on("/api/zigbee/permit-join", HTTP_POST,
        [this](AsyncWebServerRequest*) {},
        nullptr,
        [this](AsyncWebServerRequest* req, uint8_t* data, size_t len,
                size_t, size_t) {
            JsonDocument body;
            if (deserializeJson(body, reinterpret_cast<const char*>(data), len)) {
                req->send(400, "application/json", "{\"error\":\"bad_json\"}");
                return;
            }
            if (!body["duration_s"].is<int>()) {
                req->send(400, "application/json",
                          "{\"error\":\"missing_duration\"}");
                return;
            }
            const int dur = body["duration_s"].as<int>();
            if (dur < 1 || dur > 254) {
                req->send(400, "application/json",
                          "{\"error\":\"bad_duration\"}");
                return;
            }
            const auto rc = zb_.permitJoin(static_cast<uint8_t>(dur));
            if (rc != gh::domain::ErrorCode::Ok) {
                req->send(500, "application/json",
                          "{\"error\":\"permit_join_failed\"}");
                return;
            }
            JsonDocument out;
            out["ok"]         = true;
            out["duration_s"] = dur;
            auto* resp = req->beginResponseStream("application/json");
            serializeJson(out, *resp);
            req->send(resp);
        }).addMiddleware(&auth_);
}

}  // namespace gh::presentation

#endif  // ARDUINO
