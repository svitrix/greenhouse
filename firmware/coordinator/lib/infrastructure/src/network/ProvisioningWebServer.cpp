#include "ProvisioningWebServer.hpp"
#include "provisioning_html.hpp"
#include "entities/WifiCreds.hpp"
#include "entities/MqttCreds.hpp"
#include "entities/SoilCalibration.hpp"
#include "platform/PasswordHasher.hpp"
#include <WiFi.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <AsyncJson.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <Arduino.h>
#include <cstring>
#include <cstdio>

namespace gh::infra {

namespace {
constexpr const char* kCaptiveRedirectHtml =
    "<!DOCTYPE html><html><head>"
    "<title>Greenhouse setup</title>"
    "<meta http-equiv=\"refresh\" content=\"0;url=http://192.168.4.1/\">"
    "</head><body>Setup</body></html>";

void copyArg(AsyncWebServerRequest* req, const char* name,
             char* dst, size_t dst_size) {
    if (req->hasParam(name, true)) {
        const auto& v = req->getParam(name, true)->value();
        std::strncpy(dst, v.c_str(), dst_size - 1);
        dst[dst_size - 1] = '\0';
    } else {
        dst[0] = '\0';
    }
}

void sendJsonError(AsyncWebServerRequest* req, int code, const char* msg) {
    char buf[160];
    std::snprintf(buf, sizeof(buf), "{\"error\":\"%s\"}", msg);
    req->send(code, "application/json", buf);
}

void restartTaskFn(void*) {
    vTaskDelay(pdMS_TO_TICKS(3000));
    ESP.restart();
}

constexpr const char* lastErrorToString(gh::domain::ConnectError e) noexcept {
    switch (e) {
        case gh::domain::ConnectError::AuthFail:     return "auth_fail";
        case gh::domain::ConnectError::SsidNotFound: return "ssid_not_found";
        case gh::domain::ConnectError::Timeout:      return "timeout";
        case gh::domain::ConnectError::Other:        return "other";
        case gh::domain::ConnectError::None:
        default:                                     return "none";
    }
}
}

ProvisioningWebServer::ProvisioningWebServer(
    gh::domain::IWifiCredsStore&         wifi,
    gh::domain::IMqttCredsStore&         mqtt,
    gh::domain::ISoilCalibrationStore&   soil,
    gh::domain::IProvisioningFlagStore&  prov_flag,
    gh::domain::ILastConnectErrorStore&  last_error,
    gh::domain::IAdminCredsStore&        admin_creds,
    gh::domain::IAnalyticsConfigStore&   analytics,
    gh::domain::IPairingClient&          pairing,
    gh::domain::ISystemInfo&             sysinfo,
    gh::domain::ILogger&                 log) noexcept
    : wifi_(wifi), mqtt_(mqtt), soil_(soil),
      prov_flag_(prov_flag), last_error_(last_error),
      admin_creds_(admin_creds), analytics_(analytics),
      pairing_(pairing),
      sysinfo_(sysinfo), log_(log) {}

void ProvisioningWebServer::start() noexcept {
    server_.on("/", HTTP_GET, [](AsyncWebServerRequest* req) {
        if (LittleFS.exists("/index.html.gz")) {
            auto* resp = req->beginResponse(LittleFS, "/index.html.gz", "text/html");
            resp->addHeader("Content-Encoding", "gzip");
            resp->addHeader("Cache-Control", "no-store");
            req->send(resp);
            return;
        }
        if (LittleFS.exists("/index.html")) {
            auto* resp = req->beginResponse(LittleFS, "/index.html", "text/html");
            resp->addHeader("Cache-Control", "no-store");
            req->send(resp);
            return;
        }
        // Fallback for first install before LittleFS image is uploaded
        req->send_P(200, "text/html", kProvisioningHtml);
    });

    // Async Wi-Fi scan. The first GET triggers a background scan
    // (WIFI_SCAN_RUNNING == -1 while it runs). Subsequent GETs return:
    //   202 with {"status":"scanning"} while WiFi.scanComplete() == -1
    //   200 with networks[]            when scanComplete() >= 0
    // Re-triggered every kScanRefreshMs to keep results reasonably fresh.
    server_.on("/scan", HTTP_GET, [this](AsyncWebServerRequest* req) {
        constexpr uint32_t kScanRefreshMs = 30'000U;
        const uint32_t now = millis();
        const int complete = WiFi.scanComplete();

        const bool need_new_scan =
            (complete == WIFI_SCAN_FAILED) ||
            (last_scan_trigger_ms_ == 0U) ||
            (complete >= 0 && (now - last_scan_trigger_ms_) > kScanRefreshMs);

        if (need_new_scan) {
            WiFi.scanDelete();
            WiFi.scanNetworks(/*async*/ true, /*show_hidden*/ false);
            last_scan_trigger_ms_ = now;
        }

        const int n = WiFi.scanComplete();
        if (n == WIFI_SCAN_RUNNING || n == WIFI_SCAN_FAILED) {
            req->send(202, "application/json",
                      "{\"status\":\"scanning\",\"retry_after_ms\":500}");
            return;
        }

        auto* resp = new AsyncJsonResponse();
        JsonObject root = resp->getRoot().to<JsonObject>();
        JsonArray  arr  = root["networks"].to<JsonArray>();
        for (int i = 0; i < n; ++i) {
            JsonObject obj = arr.add<JsonObject>();
            obj["ssid"] = WiFi.SSID(i);
            obj["rssi"] = WiFi.RSSI(i);
        }
        resp->setLength();
        req->send(resp);
    });

    server_
        .serveStatic("/assets/", LittleFS, "/assets/")
        .setCacheControl("public, max-age=31536000, immutable");
    server_
        .serveStatic("/icons.svg", LittleFS, "/icons.svg")
        .setCacheControl("public, max-age=86400");

    server_.on("/api/status", HTTP_GET, [this](AsyncWebServerRequest* req) {
        gh::domain::SystemInfo info{};
        sysinfo_.snapshot(info);
        // Provisioning-mode status: STA / MQTT are intentionally absent — the
        // operational RestApi serves the rich /api/status in Phase C+.
        // last_connect_error surfaces the prior STA disconnect reason so the
        // SPA can render an actionable banner ("wrong password" vs "timeout").
        const char* lc = lastErrorToString(last_error_.load());
        char buf[320];
        std::snprintf(buf, sizeof(buf),
            "{\"device_id\":\"%s\",\"uptime_s\":%lu,"
            "\"firmware_version\":\"%s\","
            "\"ip\":\"192.168.4.1\",\"mode\":\"provisioning\","
            "\"last_connect_error\":\"%s\"}",
            info.device_id,
            static_cast<unsigned long>(info.uptime_s),
            info.firmware_version,
            lc);
        auto* resp = req->beginResponse(200, "application/json", buf);
        resp->addHeader("Cache-Control", "no-store");
        req->send(resp);
    });

    server_.on("/save", HTTP_POST, [this](AsyncWebServerRequest* req) {
        gh::domain::WifiCreds wc{};
        copyArg(req, "wifi_ssid",     wc.ssid,     sizeof(wc.ssid));
        copyArg(req, "wifi_password", wc.password, sizeof(wc.password));
        copyArg(req, "wifi_hostname", wc.hostname, sizeof(wc.hostname));
        if (!wc.valid()) {
            sendJsonError(req, 400, "wifi: invalid SSID");
            return;
        }

        // MQTT is optional: an empty broker host means "no Home Assistant
        // integration", and the operational path then skips publishing. Only
        // parse and validate the rest of the fields when a host was provided.
        gh::domain::MqttCreds mc{};
        copyArg(req, "mqtt_host", mc.host, sizeof(mc.host));
        const bool mqtt_submitted = (mc.host[0] != '\0');
        if (mqtt_submitted) {
            mc.port = req->hasParam("mqtt_port", true)
                ? static_cast<uint16_t>(req->getParam("mqtt_port", true)->value().toInt())
                : 1883;
            if (mc.port == 0) mc.port = 1883;
            copyArg(req, "mqtt_user",         mc.user,         sizeof(mc.user));
            copyArg(req, "mqtt_password",     mc.password,     sizeof(mc.password));
            copyArg(req, "mqtt_client_id",    mc.client_id,    sizeof(mc.client_id));
            copyArg(req, "mqtt_topic_prefix", mc.topic_prefix, sizeof(mc.topic_prefix));
            if (!mc.valid()) {
                sendJsonError(req, 400, "mqtt: invalid host or port");
                return;
            }
        }

        const bool has_dry = req->hasParam("soil_dry", true);
        const bool has_wet = req->hasParam("soil_wet", true);
        const bool soil_submitted = has_dry || has_wet;
        gh::domain::SoilCalibration sc{};
        if (soil_submitted) {
            if (!has_dry || !has_wet) {
                sendJsonError(req, 400, "soil: both soil_dry and soil_wet are required");
                return;
            }
            sc.raw_dry = static_cast<uint16_t>(
                req->getParam("soil_dry", true)->value().toInt());
            sc.raw_wet = static_cast<uint16_t>(
                req->getParam("soil_wet", true)->value().toInt());
            if (!sc.valid()) {
                sendJsonError(req, 400, "soil: dry must be < wet");
                return;
            }
        }

        // Admin creds: validate length + confirm match BEFORE hashing.
        // Saved first so a failed admin save leaves Wi-Fi / MQTT untouched
        // (the operator can re-try without clobbering known-good creds).
        char admin_user[gh::domain::AdminCreds::kUsernameMax + 1] = {};
        char admin_pass[64]         = {};
        char admin_pass_confirm[64] = {};
        copyArg(req, "admin_user",             admin_user,         sizeof(admin_user));
        copyArg(req, "admin_password",         admin_pass,         sizeof(admin_pass));
        copyArg(req, "admin_password_confirm", admin_pass_confirm, sizeof(admin_pass_confirm));

        if (std::strlen(admin_pass) < 8) {
            req->send(400, "text/html",
                      "<h2>Admin password too short</h2><p>Minimum 8 characters.</p>"
                      "<a href=\"/\">Go back</a>");
            return;
        }
        if (std::strcmp(admin_pass, admin_pass_confirm) != 0) {
            req->send(400, "text/html",
                      "<h2>Passwords do not match</h2><a href=\"/\">Go back</a>");
            return;
        }

        gh::domain::AdminCreds ac{};
        const char* user_src = (admin_user[0] != '\0') ? admin_user : "admin";
        std::strncpy(ac.username, user_src, gh::domain::AdminCreds::kUsernameMax);
        ac.username[gh::domain::AdminCreds::kUsernameMax] = '\0';
        gh::infra::generateSalt(ac.salt);
        ac.iterations = gh::infra::kPbkdf2DefaultIterations;
        gh::infra::hashPassword(admin_pass, ac.salt, ac.iterations, ac.password_hash);

        if (admin_creds_.save(ac) != gh::domain::ErrorCode::Ok) {
            req->send(500, "text/html",
                      "<h2>NVS error saving admin creds</h2><a href=\"/\">Go back</a>");
            return;
        }

        if (wifi_.save(wc) != gh::domain::ErrorCode::Ok) {
            sendJsonError(req, 500, "NVS save failed");
            return;
        }
        if (mqtt_submitted && mqtt_.save(mc) != gh::domain::ErrorCode::Ok) {
            sendJsonError(req, 500, "NVS save failed");
            return;
        }
        if (soil_submitted && soil_.save(sc) != gh::domain::ErrorCode::Ok) {
            sendJsonError(req, 500, "NVS save failed");
            return;
        }

        // Analytics + pairing (optional). Empty URL → analytics disabled
        // → clear stored config so the operational path skips the task.
        // Non-empty URL → do an inline STA-connect, POST /api/pairing/claim
        // with the 6-digit code, persist the returned api_key.
        {
            char analytics_url[sizeof(gh::domain::AnalyticsConfig::backend_url)] = {};
            char analytics_code[8] = {};
            copyArg(req, "analytics_url",  analytics_url,  sizeof(analytics_url));
            copyArg(req, "analytics_code", analytics_code, sizeof(analytics_code));

            if (analytics_url[0] != '\0') {
                if (std::strlen(analytics_code) != 6) {
                    sendJsonError(req, 400, "analytics: missing 6-digit pairing code");
                    return;
                }

                // Inline STA-connect with the freshly-saved Wi-Fi creds.
                // ~30s busy wait acceptable here (one-shot, no watchdog yet).
                WiFi.mode(WIFI_AP_STA);
                WiFi.begin(wc.ssid, wc.password);
                const uint32_t deadline = millis() + 30'000;
                while (WiFi.status() != WL_CONNECTED && millis() < deadline) {
                    delay(100);
                }
                if (WiFi.status() != WL_CONNECTED) {
                    sendJsonError(req, 503,
                        "analytics: Wi-Fi STA failed (re-check SSID/password)");
                    return;
                }

                char device_id[16] = {};
                {
                    uint8_t mac[6] = {0};
                    WiFi.macAddress(mac);
                    std::snprintf(device_id, sizeof(device_id),
                                  "gh-%02x%02x%02x", mac[3], mac[4], mac[5]);
                }
                char mac_str[18] = {};
                {
                    uint8_t mac[6] = {0};
                    WiFi.macAddress(mac);
                    std::snprintf(mac_str, sizeof(mac_str),
                                  "%02x:%02x:%02x:%02x:%02x:%02x",
                                  mac[0], mac[1], mac[2],
                                  mac[3], mac[4], mac[5]);
                }

                char api_key[96] = {};
                const auto err = pairing_.claim(
                    analytics_url, analytics_code,
                    device_id, mac_str, "0.4.0", "gh-coordinator-v1",
                    api_key, sizeof(api_key)
                );
                if (err != gh::domain::ErrorCode::Ok) {
                    const char* msg = "analytics: claim failed";
                    switch (err) {
                        case gh::domain::ErrorCode::PairingWindowExpired:
                            msg = "analytics: pairing code expired or unknown"; break;
                        case gh::domain::ErrorCode::PairingDeviceConflict:
                            msg = "analytics: device already registered (admin must revoke first)"; break;
                        case gh::domain::ErrorCode::PairingProfileUnknown:
                            msg = "analytics: profile not supported by backend (upgrade backend)"; break;
                        case gh::domain::ErrorCode::HttpTransportFailure:
                        case gh::domain::ErrorCode::HttpTimeout:
                            msg = "analytics: backend unreachable"; break;
                        default: break;
                    }
                    sendJsonError(req, 502, msg);
                    return;
                }

                gh::domain::AnalyticsConfig acfg{};
                std::strncpy(acfg.backend_url, analytics_url,
                             sizeof(acfg.backend_url) - 1);
                std::strncpy(acfg.api_key, api_key,
                             sizeof(acfg.api_key) - 1);
                acfg.flush_period_s = 900;
                acfg.insecure_tls   = false;
                if (analytics_.save(acfg) != gh::domain::ErrorCode::Ok) {
                    sendJsonError(req, 500, "NVS analytics save failed");
                    return;
                }
            } else {
                analytics_.clear();
            }
        }

        (void)prov_flag_.setForced(false);  // clear so next boot enters operational mode
        log_.info("provisioning", "all creds saved, restarting");

        const char* hostname = (wc.hostname[0] != '\0') ? wc.hostname : "greenhouse";
        char host_local[40];
        std::snprintf(host_local, sizeof(host_local), "%s.local", hostname);

        char page[sizeof(kProvisioningSuccessHtml) + 64 + 40];
        std::snprintf(page, sizeof(page), kProvisioningSuccessHtml,
                      host_local, ac.username);
        req->send(200, "text/html", page);

        xTaskCreate(restartTaskFn, "restart_task", 2048, nullptr, 1, nullptr);
    });

    server_.on("/status", HTTP_GET, [](AsyncWebServerRequest* req) {
        req->send(200, "application/json",
                  "{\"mode\":\"provisioning\",\"ip\":\"192.168.4.1\"}");
    });

    registerCaptiveProbes_(server_);

    server_.onNotFound([](AsyncWebServerRequest* req) {
        req->redirect("http://192.168.4.1/");
    });

    server_.begin();
}

void ProvisioningWebServer::registerCaptiveProbes_(AsyncWebServer& s) noexcept {
    // Apple iOS/macOS Captive Network Assistant + Firefox — need 200 with body
    // that is NOT the OS-specific "Success" sentinel.
    auto apple_firefox = [](AsyncWebServerRequest* req) {
        req->send(200, "text/html", kCaptiveRedirectHtml);
    };
    s.on("/hotspot-detect.html",       HTTP_GET, apple_firefox);
    s.on("/library/test/success.html", HTTP_GET, apple_firefox);
    s.on("/canonical.html",            HTTP_GET, apple_firefox);

    // Android + Windows — need 302 (NOT 204 / NOT plain 200).
    auto redirect_302 = [](AsyncWebServerRequest* req) {
        AsyncWebServerResponse* r = req->beginResponse(302, "text/plain", "");
        r->addHeader("Location", "http://192.168.4.1/");
        req->send(r);
    };
    s.on("/generate_204",    HTTP_GET, redirect_302);
    s.on("/connecttest.txt", HTTP_GET, redirect_302);
    s.on("/ncsi.txt",        HTTP_GET, redirect_302);
}

}
