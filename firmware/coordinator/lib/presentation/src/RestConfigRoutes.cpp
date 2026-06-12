#ifdef ARDUINO

#include "RestConfigRoutes.hpp"
#include "RestHelpers.hpp"
#include "entities/AutoWaterConfig.hpp"
#include "entities/MqttCreds.hpp"
#include "entities/WifiCreds.hpp"
#include <ArduinoJson.h>
#include <cstring>

namespace gh::presentation {

namespace {

void writeAutoWaterJson(JsonObject root, const gh::domain::AutoWaterConfig& aw) {
    root["enabled"]            = aw.enabled;
    root["trigger_below_pct"]  = aw.trigger_below_pct;
    root["min_interval_min"]   = aw.min_interval_min;
    root["duration_s"]         = aw.duration_s;
    root["min_fresh_sources"]  = aw.min_fresh_sources;
    root["stale_threshold_s"]  = aw.stale_threshold_s;
}

void writeMqttJson(JsonObject root, const gh::domain::MqttCreds& mc) {
    root["host"]         = mc.host;
    root["port"]         = mc.port;
    root["user"]         = mc.user;
    root["password_set"] = mc.password[0] != '\0';
}

void writeWifiJson(JsonObject root, const gh::domain::WifiCreds& wc) {
    root["ssid"] = wc.ssid;
}

void mergeAutoWater(JsonObjectConst j, gh::domain::AutoWaterConfig& aw) {
    if (j["enabled"].is<bool>())            aw.enabled = j["enabled"].as<bool>();
    if (j["trigger_below_pct"].is<int>())   aw.trigger_below_pct =
        static_cast<uint8_t>(j["trigger_below_pct"].as<int>());
    if (j["min_interval_min"].is<int>())    aw.min_interval_min =
        static_cast<uint16_t>(j["min_interval_min"].as<int>());
    if (j["duration_s"].is<int>())          aw.duration_s =
        static_cast<uint8_t>(j["duration_s"].as<int>());
    if (j["min_fresh_sources"].is<int>())   aw.min_fresh_sources =
        static_cast<uint8_t>(j["min_fresh_sources"].as<int>());
    if (j["stale_threshold_s"].is<int>())   aw.stale_threshold_s =
        static_cast<uint32_t>(j["stale_threshold_s"].as<int>());
}

void mergeMqtt(JsonObjectConst j, gh::domain::MqttCreds& mc) {
    if (j["host"].is<const char*>()) {
        std::strncpy(mc.host, j["host"].as<const char*>(), sizeof(mc.host) - 1);
        mc.host[sizeof(mc.host) - 1] = '\0';
    }
    if (j["port"].is<int>()) mc.port = static_cast<uint16_t>(j["port"].as<int>());
    if (j["user"].is<const char*>()) {
        std::strncpy(mc.user, j["user"].as<const char*>(), sizeof(mc.user) - 1);
        mc.user[sizeof(mc.user) - 1] = '\0';
    }
    if (j["password"].is<const char*>()) {
        const char* p = j["password"].as<const char*>();
        if (std::strlen(p) > 0) {
            std::strncpy(mc.password, p, sizeof(mc.password) - 1);
            mc.password[sizeof(mc.password) - 1] = '\0';
        }
    }
}

}  // namespace

bool RestConfigRoutes::applyAutoWater(AsyncWebServerRequest* req,
                                      JsonObjectConst j) noexcept {
    auto cur = auto_water_store_.load();
    gh::domain::AutoWaterConfig aw =
        cur.ok() ? cur.value : gh::domain::kDefaultAutoWaterConfig;
    mergeAutoWater(j, aw);
    if (!aw.valid()) {
        rest::sendError(req, 400, "bad_auto_water", "invalid auto-water config");
        return false;
    }
    if (auto_water_store_.save(aw) != gh::domain::ErrorCode::Ok) {
        rest::sendError(req, 500, "nvs_error", "failed to persist auto-water config");
        return false;
    }
    return true;
}

bool RestConfigRoutes::applyMqtt(AsyncWebServerRequest* req,
                                 JsonObjectConst j) noexcept {
    auto cur = mqtt_store_.load();
    gh::domain::MqttCreds mc = cur.ok() ? cur.value : gh::domain::MqttCreds{};
    mergeMqtt(j, mc);
    if (!mc.valid()) {
        rest::sendError(req, 400, "bad_mqtt", "invalid MQTT credentials");
        return false;
    }
    if (mqtt_store_.save(mc) != gh::domain::ErrorCode::Ok) {
        rest::sendError(req, 500, "nvs_error", "failed to persist MQTT credentials");
        return false;
    }
    return true;
}

void RestConfigRoutes::registerOn(AsyncWebServer& server) noexcept {
    server.on("/api/config", HTTP_GET,
        [this](AsyncWebServerRequest* req) {
            auto aw_r = auto_water_store_.load();
            auto mc_r = mqtt_store_.load();
            auto wc_r = wifi_store_.load();
            const gh::domain::AutoWaterConfig aw =
                aw_r.ok() ? aw_r.value : gh::domain::kDefaultAutoWaterConfig;
            const gh::domain::MqttCreds mc =
                mc_r.ok() ? mc_r.value : gh::domain::MqttCreds{};
            const gh::domain::WifiCreds wc =
                wc_r.ok() ? wc_r.value : gh::domain::WifiCreds{};

            auto* resp = req->beginResponseStream("application/json");
            resp->addHeader("Cache-Control", "no-store");
            JsonDocument doc;
            writeAutoWaterJson(doc["auto_water"].to<JsonObject>(), aw);
            writeMqttJson    (doc["mqtt"].to<JsonObject>(),       mc);
            writeWifiJson    (doc["wifi"].to<JsonObject>(),       wc);
            serializeJson(doc, *resp);
            req->send(resp);
        });

    server.on("/api/config", HTTP_POST,
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
            if (body["auto_water"].is<JsonObject>() &&
                !applyAutoWater(req, body["auto_water"].as<JsonObjectConst>())) {
                return;
            }
            if (body["mqtt"].is<JsonObject>() &&
                !applyMqtt(req, body["mqtt"].as<JsonObjectConst>())) {
                return;
            }
            auto* resp = req->beginResponseStream("application/json");
            resp->addHeader("Cache-Control", "no-store");
            JsonDocument out;
            out["ok"]               = true;
            out["restart_required"] = false;
            serializeJson(out, *resp);
            req->send(resp);
        });
}

}  // namespace gh::presentation

#endif  // ARDUINO
