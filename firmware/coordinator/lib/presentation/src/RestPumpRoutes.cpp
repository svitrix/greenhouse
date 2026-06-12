#ifdef ARDUINO

#include "RestPumpRoutes.hpp"
#include "RestHelpers.hpp"
#include "PumpStateView.hpp"
#include "irrigation/AutoWaterDecision.hpp"
#include "entities/PumpState.hpp"
#include <ArduinoJson.h>
#include <cstring>

namespace gh::presentation {

namespace {

void sendPumpState(AsyncWebServerRequest* req, gh::domain::PumpState state) {
    auto* resp = req->beginResponseStream("application/json");
    resp->addHeader("Cache-Control", "no-store");
    JsonDocument doc;
    doc["state"]       = pumpStateCode(state);
    doc["remaining_s"] = 0;
    doc["last_run_ms"] = 0;
    serializeJson(doc, *resp);
    req->send(resp);
}
}  // namespace

void RestPumpRoutes::registerOn(AsyncWebServer& server) noexcept {
    server.on("/api/pump", HTTP_GET,
        [this](AsyncWebServerRequest* req) {
            sendPumpState(req, pump_.state());
        });

    server.on("/api/pump", HTTP_POST,
        [](AsyncWebServerRequest*) {},
        nullptr,
        [this](AsyncWebServerRequest* req, uint8_t* data, size_t len,
                size_t index, size_t total) {
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
            const char* st = body["state"] | "";
            gh::app::AutoWaterDecision d{};
            if (std::strcmp(st, "ON") == 0) {
                d = irrigation_.requestOn();
            } else if (std::strcmp(st, "OFF") == 0) {
                d = irrigation_.requestOff();
            } else {
                rest::sendError(req, 400, "bad_state", "state must be ON or OFF");
                return;
            }

            const bool ok =
                d.outcome == gh::app::AutoWaterOutcome::Started ||
                d.outcome == gh::app::AutoWaterOutcome::Stopped;
            auto* resp = req->beginResponseStream("application/json");
            resp->setCode(ok ? 200 : 409);
            resp->addHeader("Cache-Control", "no-store");
            JsonDocument out;
            out["state"]   = pumpStateCode(pump_.state());
            out["outcome"] = gh::app::outcomeCode(d.outcome);
            out["ok"]      = ok;
            if (!ok) {
                out["lockout_reason"] = gh::app::outcomeCode(d.outcome);
            }
            serializeJson(out, *resp);
            req->send(resp);
        });
}

}  // namespace gh::presentation

#endif  // ARDUINO
