#include <Arduino.h>
#include <ESPmDNS.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <WiFi.h>
#include <mbedtls/base64.h>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <sys/time.h>
#include "esp_task_wdt.h"
#include "esp_mac.h"
#include "esp_coexist.h"
#include "AnalyticsUploader.hpp"
#include "AppConfig.hpp"
#include "CoordinatorConfig.hpp"
#include "entities/AutoWaterConfig.hpp"
#include "ports/IAdminCredsStore.hpp"
#include "WifiProvisioner.hpp"
#include "ports/IWifiFailCounterStore.hpp"
#include "ports/ILastConnectErrorStore.hpp"
#include "persistence/LittleFsTelemetryQueue.hpp"
#include "persistence/NvsAdminCredsStore.hpp"
#include "persistence/NvsAnalyticsConfigStore.hpp"
#include "persistence/NvsAutoWaterConfigStore.hpp"
#include "persistence/NvsLastConnectErrorStore.hpp"
#include "persistence/NvsMqttCredsStore.hpp"
#include "persistence/NvsProvisioningFlagStore.hpp"
#include "persistence/NvsSoilCalibrationStore.hpp"
#include "persistence/NvsWifiCredsStore.hpp"
#include "persistence/NvsWifiFailCounterStore.hpp"
#include "persistence/NvsZigbeeNetStore.hpp"
#include "persistence/NvsNodeAliasStore.hpp"
#include "platform/SerialLogger.hpp"
#include "platform/ArduinoClock.hpp"
#include "platform/ArduinoSystemInfo.hpp"
#include "platform/GpioButton.hpp"
#include "platform/PasswordHasher.hpp"
#include "network/WifiStaAdapter.hpp"
#include "network/WifiSoftApAdapter.hpp"
#include "network/CaptiveDnsServer.hpp"
#include "network/ProvisioningWebServer.hpp"
#include "network/ZigbeeCoordinatorAdapter.hpp"
#include "network/ZigbeeBindingTable.hpp"
#include "network/Esp32ZigbeeNetwork.hpp"
#include "network/EspHttpsClient.hpp"
#include "network/EspPairingClient.hpp"
#include "network/MqttClient.hpp"
#include "registry/InMemoryNodeRegistry.hpp"
#include "registry/InMemoryHistoryStore.hpp"
#include "ZigbeeNetwork.hpp"
#include "drivers/RelayPump.hpp"
#include "drivers/FakeFloatSwitchAlwaysOk.hpp"
#include "drivers/Ws2812StatusLed.hpp"
#include "status/StatusLedService.hpp"
#include "MqttCommandRouter.hpp"
#include "zigbee/ZigbeeReportRouter.hpp"
#include "telemetry/TelemetryPublisher.hpp"
#include "telemetry/V1MqttPurge.hpp"
#include "irrigation/IrrigationService.hpp"
#include "node/NodePruneService.hpp"
#include "HomeAssistantDiscoveryService.hpp"
#include "RestNodesRoutes.hpp"
#include "RestNodeAliasRoutes.hpp"
#include "RestNodeDeleteRoutes.hpp"
#include "RestHistoryRoutes.hpp"
#include "RestPumpRoutes.hpp"
#include "RestStatusRoutes.hpp"
#include "RestConfigRoutes.hpp"
#include "RestZigbeeRoutes.hpp"
#include "RestAutoWaterRoutes.hpp"
#include "DashboardViewBuilder.hpp"

using cfg = gh::app::AppConfig;

namespace {

constexpr const char* kFirmwareVersion = "0.4.0";

// Shared task context. espMqttClient and xTaskCreate take C callbacks without
// a `user_ctx`, so we route through one file-scope struct that each task
// entry point dereferences from `pvParameters`. Populated in runOperational()
// before any xTaskCreate; never mutated afterwards.
struct TaskCtx {
    gh::infra::MqttClient*                                  mqtt         = nullptr;
    gh::app::IrrigationService*                           irrigation   = nullptr;
    gh::app::TelemetryPublisher*                          telemetry    = nullptr;
    gh::app::NodePruneService*                              prune        = nullptr;
    gh::app::AnalyticsUploader*                             analytics    = nullptr;
    gh::presentation::HomeAssistantDiscoveryService*  ha           = nullptr;
    gh::presentation::MqttCommandRouter*                    cmd_router   = nullptr;
    gh::infra::ZigbeeCoordinatorAdapter*                    zb           = nullptr;
    gh::domain::IPump*                                      pump         = nullptr;
    gh::app::StatusLedService*                              led          = nullptr;
    gh::domain::INodeRegistry*                              reg          = nullptr;
    gh::domain::INodeAliasStore*                            aliases      = nullptr;
    gh::domain::IClock*                                     clk          = nullptr;
    AsyncEventSource*                                       events       = nullptr;
    bool                                                    mqtt_expected = false;
    const char*                                             device_id_cstr = nullptr;
};

TaskCtx s_ctx{};

// Status LED lives at composition-root scope so setup() can show
// WifiConnecting before the mode is decided, and both runProvisioning() and
// runOperational() drive the same instance. Construction touches no hardware.
gh::infra::Ws2812StatusLed s_status_led{
    gh::coord::CoordinatorConfig::kStatusLedGpio,
    gh::coord::CoordinatorConfig::kStatusLedBrightnessPct};
gh::app::StatusLedService  s_led{s_status_led};

// Coordinator main work loop. Drives the v2 service ticks at 0.2 Hz.
[[noreturn]] void coordinator_task(void* pv) {
    auto& ctx = *static_cast<TaskCtx*>(pv);
    esp_task_wdt_add(NULL);
    uint8_t wifi_down_ticks = 0;  // 5 s per tick
    for (;;) {
        esp_task_wdt_reset();
        if (ctx.irrigation) (void)ctx.irrigation->tick();
        if (ctx.prune)      ctx.prune->tick();
        if (ctx.telemetry)  ctx.telemetry->tick();
        if (ctx.ha)         ctx.ha->reconcile();
        // Push the desired report period to any newly-seen sensor.
        if (ctx.zb)         (void)ctx.zb->drainPendingPeriodWrites(cfg::kReportPeriodS);
        // Supervisory Wi-Fi recovery: belt-and-suspenders for the case where a
        // link drop fired no event the handler/autoreconnect caught. After ~60 s
        // offline, force a reconnect attempt.
        if (WiFi.status() == WL_CONNECTED) {
            wifi_down_ticks = 0;
        } else if (++wifi_down_ticks >= 12) {
            WiFi.reconnect();
            wifi_down_ticks = 0;
        }
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

[[noreturn]] void dns_task(void* pv) {
    auto* dns = static_cast<gh::infra::CaptiveDnsServer*>(pv);
    for (;;) {
        dns->processNext();
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

[[noreturn]] void watchdog_task(void* /*pv*/) {
    // The hardware watchdog is reset inside each enrolled task. This task
    // exists only to provide a free-running 30 s heartbeat for future
    // anomaly detectors. Currently a no-op.
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(30'000));
    }
}

[[noreturn]] void mqtt_task(void* pv) {
    auto& ctx = *static_cast<TaskCtx*>(pv);
    // espMqttClient drives the socket on the AsyncTCP task; this task only
    // watches for (re)connect transitions, runs the one-shot v1 retained
    // cleanup, and triggers the command-router subscribe on each (re)connect.
    bool subscribed = false;
    esp_task_wdt_add(NULL);
    for (;;) {
        esp_task_wdt_reset();
        if (ctx.mqtt->isConnected()) {
            if (!subscribed) {
                if (ctx.cmd_router) ctx.cmd_router->subscribe();
                subscribed = true;
            }
            if (ctx.device_id_cstr) {
                // V1MqttPurge is idempotent (NVS-gated). Calling it every loop
                // until it succeeds covers the case where MQTT is up but the
                // retain-clear publishes haven't been delivered yet.
                (void)gh::app::V1MqttPurge::runIfNeeded(*ctx.mqtt, ctx.device_id_cstr);
            }
        } else {
            subscribed = false;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

[[noreturn]] void analytics_task(void* pv) {
    auto& ctx = *static_cast<TaskCtx*>(pv);
    // tick() is cheap; the period_ms gate inside AnalyticsUploader decides
    // whether a flush actually happens. 1 Hz polling is fine.
    esp_task_wdt_add(NULL);
    for (;;) {
        esp_task_wdt_reset();
        if (ctx.analytics) ctx.analytics->tick();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// Operational status LED: re-derive the highest-priority status from runtime
// signals (pump state, MQTT link) and advance the blink phase. Not enrolled in
// the watchdog — purely cosmetic, must never reset the chip.
[[noreturn]] void led_task(void* pv) {
    auto& ctx = *static_cast<TaskCtx*>(pv);
    for (;;) {
        if (ctx.led) {
            const auto pump_state = ctx.pump ? ctx.pump->state()
                                             : gh::domain::PumpState::Off;
            const bool wifi_up = WiFi.status() == WL_CONNECTED;
            const bool mqtt_up = ctx.mqtt && ctx.mqtt->isConnected();
            const bool pairing = ctx.zb && ctx.zb->isPermitJoinOpen();
            ctx.led->setStatus(gh::app::StatusLedService::arbitrate(
                pump_state, wifi_up, mqtt_up, ctx.mqtt_expected, pairing));
            ctx.led->tick(millis());
        }
        vTaskDelay(pdMS_TO_TICKS(gh::coord::CoordinatorConfig::kLedTickMs));
    }
}

// SSE push: every 2 s, while at least one client is connected (and not backed
// up), serialize the combined dashboard payload and push it to /api/events.
// Replaces the SPA's 3-GETs-every-2s polling with one persistent connection —
// strictly less radio churn on the Wi-Fi/Zigbee-shared 2.4 GHz front end.
// Not WDT-enrolled (cosmetic; a stalled TCP flush must never reset the chip).
[[noreturn]] void sse_task(void* pv) {
    auto& ctx = *static_cast<TaskCtx*>(pv);
    String buf;
    for (;;) {
        if (ctx.events && ctx.events->count() > 0 &&
            ctx.events->avgPacketsWaiting() < 3 &&
            ctx.reg && ctx.aliases && ctx.clk && ctx.pump && ctx.irrigation) {
            JsonDocument doc;
            JsonObject root = doc.to<JsonObject>();
            gh::presentation::DashboardViewBuilder::build(
                *ctx.reg, *ctx.aliases, ctx.pump->state(),
                ctx.irrigation->lastDecision(), ctx.clk->nowMs(), root);
            buf.clear();
            serializeJson(doc, buf);
            ctx.events->send(buf.c_str(), "dashboard", millis(), 5000);
        }
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

// Provisioning-mode LED: the status is fixed (set before the task starts), so
// this just advances the blink phase.
[[noreturn]] void led_tick_task(void* pv) {
    auto* led = static_cast<gh::app::StatusLedService*>(pv);
    for (;;) {
        led->tick(millis());
        vTaskDelay(pdMS_TO_TICKS(gh::coord::CoordinatorConfig::kLedTickMs));
    }
}

void runOperational(gh::infra::SerialLogger&              log,
                    gh::infra::NvsSoilCalibrationStore&  /*soilStore*/,
                    gh::infra::NvsMqttCredsStore&        mqttStore,
                    gh::infra::NvsWifiCredsStore&         wifiStore,
                    gh::infra::NvsProvisioningFlagStore&  /*prov_flag_store*/,
                    gh::infra::NvsAdminCredsStore&        admin_creds_store,
                    gh::infra::NvsAnalyticsConfigStore&  analyticsStore,
                    gh::infra::ArduinoSystemInfo&         sysinfo,
                    const std::string&                   device_id) {
    // --- Task WDT: reconfigure to 30 s ---
    {
        constexpr uint32_t kWdtTimeoutMs = 30'000;
        esp_task_wdt_config_t wdt_cfg{
            .timeout_ms      = kWdtTimeoutMs,
            .idle_core_mask  = 0,
            .trigger_panic   = true,
        };
        esp_task_wdt_reconfigure(&wdt_cfg);
    }

    // --- mDNS responder ---
    {
        const auto wifiLoaded = wifiStore.load();
        const char* host = (wifiLoaded.ok() && wifiLoaded.value.hostname[0] != '\0')
                               ? wifiLoaded.value.hostname
                               : "greenhouse";
        if (MDNS.begin(host)) {
            MDNS.addService("http", "tcp", 80);
            MDNS.addServiceTxt("http", "tcp", "device_id", device_id.c_str());
            log.info("mdns", "responder up");
        } else {
            log.warn("mdns", "responder failed to start");
        }
    }

    // --- Shared platform services ---
    static gh::infra::ArduinoClock clock{};
    s_ctx.device_id_cstr = device_id.c_str();

    // --- v2 multi-node registry + history + alias store ---
    static gh::infra::InMemoryNodeRegistry  node_registry{};
    static gh::infra::InMemoryHistoryStore  history_store{};
    static gh::infra::NvsNodeAliasStore     alias_store{};
    if (alias_store.begin() != gh::domain::ErrorCode::Ok) {
        log.warn("init", "nodes_alias NVS namespace failed to open");
    }
    static gh::infra::ZigbeeBindingTable    binding_table{};

    // --- AutoWaterConfig snapshot (NVS-backed) ---
    static gh::infra::NvsAutoWaterConfigStore auto_water_store{};
    gh::domain::AutoWaterConfig auto_water_cfg = gh::domain::kDefaultAutoWaterConfig;
    {
        const auto loaded = auto_water_store.load();
        if (loaded.ok()) auto_water_cfg = loaded.value;
    }

    // --- MQTT client ---
    const auto mqttLoaded = mqttStore.load();
    if (!mqttLoaded.ok()) {
        log.warn("mqtt", "no MQTT creds in NVS — publishing disabled");
    }
    static gh::domain::MqttCreds mqtt_creds =
        mqttLoaded.ok() ? mqttLoaded.value : gh::domain::MqttCreds{};
    static gh::infra::MqttClient mqtt{mqtt_creds};
    s_ctx.mqtt = &mqtt;
    s_ctx.mqtt_expected = mqtt_creds.valid();  // drives the status LED "Degraded" state
    (void)mqtt.connect();  // first attempt; mqtt_task handles reconnects

    // --- Pump driver ---
    static gh::infra::ArduinoGpio  arduino_gpio{};
    static gh::infra::RelayPump    pump{arduino_gpio,
                                        gh::coord::CoordinatorConfig::kRelayIn1Pin};
    s_ctx.pump = &pump;

    // --- Float switch (MVP placeholder — always reports water present) ---
    static gh::infra::FakeFloatSwitchAlwaysOk float_switch{};

    // --- v2 IrrigationService ---
    static gh::app::IrrigationService irrigation_service{
        node_registry, pump, float_switch, clock, log, auto_water_cfg,
        /*max_runtime_ms*/ 20'000U};
    s_ctx.irrigation = &irrigation_service;

    // --- v2 TelemetryPublisher + HA Discovery + NodePrune ---
    static gh::app::TelemetryPublisher telemetry{
        mqtt, node_registry, clock, device_id.c_str()};
    s_ctx.telemetry = &telemetry;

    static gh::presentation::HomeAssistantDiscoveryService ha{
        mqtt, node_registry, alias_store, device_id.c_str()};
    s_ctx.ha = &ha;

    static gh::app::NodePruneService prune{
        node_registry, clock, /*offline_ms*/ 180'000U};
    s_ctx.prune = &prune;

    // --- Zigbee identity (randomised + persisted on first boot) ---
    static gh::infra::NvsZigbeeNetStore zb_net_store{};
    const auto ext_pan_id = zb_net_store.loadOrGenerate();
    log.info("zb", "extpanid loaded");
    char ext_pan_hex[20] = {0};
    snprintf(ext_pan_hex, sizeof(ext_pan_hex),
        "%02x%02x%02x%02x%02x%02x%02x%02x",
        ext_pan_id[0], ext_pan_id[1], ext_pan_id[2], ext_pan_id[3],
        ext_pan_id[4], ext_pan_id[5], ext_pan_id[6], ext_pan_id[7]);
    log.info("zb", ext_pan_hex);
    char tc_prefix[16] = {0};
    snprintf(tc_prefix, sizeof(tc_prefix), "tc-key %02x%02x%02x%02x",
             gh::protocol::kZigbeeTcLinkKey[0],
             gh::protocol::kZigbeeTcLinkKey[1],
             gh::protocol::kZigbeeTcLinkKey[2],
             gh::protocol::kZigbeeTcLinkKey[3]);
    log.info("zb", tc_prefix);

    // --- Zigbee coordinator + report router + network adapter ---
    static gh::infra::ZigbeeCoordinatorAdapter zb{ext_pan_id};
    s_ctx.zb = &zb;
    static gh::infra::Esp32ZigbeeNetwork zb_net{zb, binding_table};
    static gh::app::ZigbeeReportRouter   zb_router{
        node_registry, history_store, binding_table, log};

    // --- MQTT command router (HA pump cmd → manual on/off) ---
    static gh::presentation::MqttCommandRouter cmd_router{
        mqtt, device_id,
        /*on_handler =*/ []() { (void)s_ctx.irrigation->requestOn();  },
        /*off_handler=*/ []() { (void)s_ctx.irrigation->requestOff(); }
    };
    s_ctx.cmd_router = &cmd_router;

    // Wire the report sink BEFORE start(): start() launches zb_task, which can
    // begin dispatching ZCL reports immediately — setting the sink afterwards
    // races the first frames.
    zb.setReportSink(&zb_router);
#ifdef GH_DIAG_NO_ZIGBEE
    (void)zb_router;
    log.warn("zigbee", "DIAG build: Zigbee start skipped (coexistence test)");
#else
    if (zb.start(gh::protocol::kInitialPermitJoinMs) != gh::domain::ErrorCode::Ok) {
        log.error("zigbee", "coordinator start failed");
        return;
    }
    log.info("zigbee", "coordinator started; report sink wired");
    // Explicitly enable Wi-Fi <-> 802.15.4 software coexistence arbitration on
    // the shared single-antenna 2.4 GHz radio. Without active arbitration the
    // always-on Zigbee coordinator RX collapses Wi-Fi throughput to ~4 KB/s and
    // large HTTP transfers (the SPA bundle) truncate. (esp_coex_preference_set
    // is deprecated and had no effect here.)
#if CONFIG_ESP_COEX_SW_COEXIST_ENABLE && CONFIG_SOC_IEEE802154_SUPPORTED
    if (esp_coex_wifi_i154_enable() != ESP_OK) {
        log.warn("coex", "wifi_i154_enable failed");
    } else {
        log.info("coex", "wifi+802.15.4 coexistence enabled");
    }
#endif
#endif

    // --- HTTP Basic Auth + REST v2 server ---
    // Auth + rate-limit are installed as *server-global* middleware so every
    // handler — including /api/dashboard, the /api/events SSE source and the
    // serveStatic SPA fallback — is protected by a single auth point. Per-route
    // .addMiddleware() is intentionally NOT used (it would double-hash).
    static AsyncWebServer server(80);
    static AsyncRateLimitMiddleware rateLimit;
    rateLimit.setMaxRequests(gh::coord::CoordinatorConfig::kAuthRateLimitMaxRequests);
    rateLimit.setWindowSize(gh::coord::CoordinatorConfig::kAuthRateLimitWindowS);
    static AsyncAuthenticationMiddleware basicAuth;
    basicAuth.setAuthType(AsyncAuthType::AUTH_BASIC);
    basicAuth.setRealm("Greenhouse Admin");
    basicAuth.setAuthFailureMessage("Authentication required");
    basicAuth.setAuthentificationFunction(
        [&admin_creds_store](AsyncWebServerRequest* req) -> bool {
            if (req->authType() != AsyncAuthType::AUTH_BASIC) {
                return false;
            }
            const String& b64 = req->authChallenge();
            if (b64.length() == 0 || b64.length() > 128) {
                return false;
            }
            uint8_t decoded[128] = {0};
            std::size_t out_len = 0;
            const int rc = mbedtls_base64_decode(
                decoded, sizeof(decoded), &out_len,
                reinterpret_cast<const uint8_t*>(b64.c_str()),
                b64.length());
            if (rc != 0 || out_len == 0 || out_len >= sizeof(decoded)) {
                return false;
            }
            decoded[out_len] = '\0';

            const auto* colon = static_cast<const uint8_t*>(
                std::memchr(decoded, ':', out_len));
            if (colon == nullptr) {
                return false;
            }
            const std::size_t user_len = static_cast<std::size_t>(colon - decoded);
            if (user_len == 0 || user_len > gh::domain::AdminCreds::kUsernameMax) {
                return false;
            }
            char user[gh::domain::AdminCreds::kUsernameMax + 1] = {0};
            std::memcpy(user, decoded, user_len);
            user[user_len] = '\0';
            const char* pass = reinterpret_cast<const char*>(colon + 1);

            auto loaded = admin_creds_store.load();
            if (!loaded.ok() || !loaded.value.valid()) {
                return false;
            }
            if (std::strcmp(user, loaded.value.username) != 0) {
                return false;
            }

            uint8_t incoming[gh::domain::AdminCreds::kHashLen];
            gh::infra::hashPassword(pass, loaded.value.salt, incoming);

            uint8_t diff = 0;
            for (std::size_t i = 0; i < gh::domain::AdminCreds::kHashLen; ++i) {
                diff = static_cast<uint8_t>(
                    diff | (incoming[i] ^ loaded.value.password_hash[i]));
            }
            return diff == 0;
        });

    // Global middleware order: rate-limit first (cheap reject of brute force),
    // then auth. Applies to every handler registered on `server`.
    server.addMiddleware(&rateLimit);
    server.addMiddleware(&basicAuth);

    static gh::presentation::RestNodesRoutes        rn_nodes  {node_registry, alias_store, clock};
    static gh::presentation::RestNodeAliasRoutes    rn_alias  {node_registry, alias_store};
    static gh::presentation::RestNodeDeleteRoutes   rn_delete {node_registry, alias_store, history_store, zb_net};
    static gh::presentation::RestHistoryRoutes    rn_hist   {history_store, clock};
    static gh::presentation::RestPumpRoutes       rn_pump   {irrigation_service, pump};
    static gh::presentation::RestStatusRoutes     rn_status {node_registry, clock, mqtt, sysinfo, device_id.c_str()};
    static gh::presentation::RestConfigRoutes     rn_cfg    {auto_water_store, mqttStore, wifiStore};
    static gh::presentation::RestZigbeeRoutes     rn_zb     {zb_net};
    static gh::presentation::RestAutoWaterRoutes  rn_aw     {irrigation_service};

    rn_nodes .registerOn(server);
    rn_alias .registerOn(server);
    rn_delete.registerOn(server);
    rn_hist  .registerOn(server);
    rn_pump  .registerOn(server);
    rn_status.registerOn(server);
    rn_cfg   .registerOn(server);
    rn_zb    .registerOn(server);
    rn_aw    .registerOn(server);

    // --- Combined dashboard payload: GET /api/dashboard (initial load /
    //     SSE-unsupported fallback) + SSE push on /api/events ---
    server.on("/api/dashboard", HTTP_GET, [&](AsyncWebServerRequest* req) {
        auto* resp = req->beginResponseStream("application/json");
        JsonDocument doc;
        JsonObject root = doc.to<JsonObject>();
        gh::presentation::DashboardViewBuilder::build(
            node_registry, alias_store, pump.state(),
            irrigation_service.lastDecision(), clock.nowMs(), root);
        serializeJson(doc, *resp);
        req->send(resp);
    });
    static AsyncEventSource events("/api/events");
    events.onConnect([&](AsyncEventSourceClient* c) {
        // Cap concurrent SSE clients (heap-bounded device); push an initial
        // snapshot so a fresh client renders immediately, not after one tick.
        if (events.count() > 2) { c->close(); return; }
        JsonDocument doc;
        JsonObject root = doc.to<JsonObject>();
        gh::presentation::DashboardViewBuilder::build(
            node_registry, alias_store, pump.state(),
            irrigation_service.lastDecision(), clock.nowMs(), root);
        String buf;
        serializeJson(doc, buf);
        c->send(buf.c_str(), "dashboard", millis(), 5000);
    });
    server.addHandler(&events);

    server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");
    server.begin();
    log.info("rest_api", "started on port 80");

    // --- SNTP (wall-clock UTC for analytics record timestamps) ---
    configTime(0, 0, "pool.ntp.org", "time.google.com");
    log.info("sntp", "configured");

    // --- Analytics uploader (optional; gated on NVS backend_url) ---
    {
        gh::domain::AnalyticsConfig acfg{};
        const auto load_res = analyticsStore.load(acfg);
        const bool url_is_https =
            (std::strncmp(acfg.backend_url, "https://", 8) == 0);
        if (load_res == gh::domain::ErrorCode::Ok && acfg.backend_url[0] != '\0'
            && !url_is_https && !acfg.insecure_tls) {
            // C2: a plain http:// hub URL would ship the Bearer api_key +
            // telemetry in cleartext. Refuse to arm the uploader unless the
            // operator explicitly set the insecure dev flag in NVS.
            log.error("analytics",
                      "backend_url is not https:// and insecure_tls=false - disabled");
        } else if (load_res == gh::domain::ErrorCode::Ok && acfg.backend_url[0] != '\0') {
            static gh::infra::LittleFsTelemetryQueue analytics_queue{};
            if (analytics_queue.begin() != gh::domain::ErrorCode::Ok) {
                log.warn("analytics", "queue begin failed - disabled");
            } else {
                // C1: thread the runtime insecure flag through. With no pinned
                // CA the client only goes insecure when insecure_tls=true;
                // otherwise it fails the TLS handshake closed.
                static gh::infra::EspHttpsClient analytics_http{
                    /*ca_cert_pem=*/nullptr,
                    /*allow_insecure_dev=*/acfg.insecure_tls};

                static char        s_device_id_buf[16] = {};
                std::snprintf(s_device_id_buf, sizeof(s_device_id_buf), "gh-%s",
                              device_id.c_str());

                static gh::app::AnalyticsUploaderConfig au_cfg{};
                au_cfg.backend_url     = acfg.backend_url;
                au_cfg.api_key         = acfg.api_key;
                au_cfg.device_id       = s_device_id_buf;
                au_cfg.fw_version      = kFirmwareVersion;
                au_cfg.flush_period_ms = acfg.flush_period_s * 1000U;

                static gh::app::AnalyticsUploader analytics{
                    analytics_queue, analytics_http, clock, log, au_cfg};
                s_ctx.analytics = &analytics;

                zb_router.setAnalyticsBridge(&analytics, &clock);
                log.info("analytics", "ZigbeeReportRouter bridge wired");

                xTaskCreate(analytics_task, "analytics_task",
                            gh::coord::CoordinatorConfig::kAnalyticsTaskStackBytes,
                            &s_ctx,
                            gh::coord::CoordinatorConfig::kAnalyticsTaskPriority,
                            nullptr);
                log.info("analytics", "uploader ENABLED");
            }
        } else {
            log.info("analytics", "disabled (no backend_url in NVS)");
        }
    }

    // --- FreeRTOS tasks ---
    xTaskCreate(coordinator_task, "coord_task",
                gh::coord::CoordinatorConfig::kCoordinatorTaskStackBytes,
                &s_ctx,
                gh::coord::CoordinatorConfig::kCoordinatorTaskPriority,
                nullptr);
    xTaskCreate(watchdog_task,    "watchdog_task", 2048, &s_ctx, 1, nullptr);
    xTaskCreate(mqtt_task,        "mqtt_task",     4096, &s_ctx, 5, nullptr);

    // --- Status LED (operational arbitration) ---
    s_ctx.led = &s_led;
    xTaskCreate(led_task, "led_task",
                gh::coord::CoordinatorConfig::kLedTaskStackBytes,
                &s_ctx,
                gh::coord::CoordinatorConfig::kLedTaskPriority,
                nullptr);

    // --- SSE dashboard push ---
    s_ctx.reg     = &node_registry;
    s_ctx.aliases = &alias_store;
    s_ctx.clk     = &clock;
    s_ctx.events  = &events;
    xTaskCreate(sse_task, "sse_task", 6144, &s_ctx, 2, nullptr);
}

void runProvisioning(gh::infra::SerialLogger&              log,
                     gh::infra::NvsWifiCredsStore&         wifiStore,
                     gh::infra::NvsMqttCredsStore&         mqttStore,
                     gh::infra::NvsSoilCalibrationStore&   soilStore,
                     gh::infra::NvsProvisioningFlagStore&  provFlagStore,
                     gh::domain::ILastConnectErrorStore&   lastErrorStore,
                     gh::infra::NvsAdminCredsStore&        adminCredsStore,
                     gh::infra::NvsAnalyticsConfigStore&   analyticsStore,
                     gh::infra::ArduinoSystemInfo&         sysinfo) {
    // Static provisioning passphrase for this stage. The per-device
    // MAC-derived passphrase is disabled until the MAC-read-before-WiFi-init
    // bug is fixed (WiFi.macAddress() returns 00:00:.. before the stack is up,
    // so every device collapsed to the same gh-00000000 anyway).
    const char* ap_passphrase = "GreenHouse";
    log.info("provisioning", "AP passphrase (note this down):");
    log.info("provisioning", ap_passphrase);

    static gh::infra::WifiSoftApAdapter ap{};
    if (const auto err = ap.start(cfg::kApSsidPrefix, ap_passphrase);
        err != gh::domain::ErrorCode::Ok) {
        log.error("softap", "failed to start");
        return;
    }

    static gh::infra::CaptiveDnsServer dns{};
    dns.start(ap.softApIP());

    // Pairing runs inside the captive-portal /save handler — a physically
    // present, operator-initiated, one-shot flow against a hub the operator
    // typed by hand (the documented local-dev example is a plain http:// URL,
    // e.g. http://192.168.1.42:8000/ingest). Allow dev-insecure here so that
    // flow keeps working; the operational analytics path stays fail-closed by
    // default (see runAnalytics composition below). No pinned CA at this stage.
    static gh::infra::EspHttpsClient pairing_https{/*ca_cert_pem=*/nullptr,
                                                   /*allow_insecure_dev=*/true};
    static gh::infra::EspPairingClient pairing_client{pairing_https};

    static gh::infra::ProvisioningWebServer web{
        wifiStore, mqttStore, soilStore, provFlagStore,
        lastErrorStore, adminCredsStore, analyticsStore,
        pairing_client, sysinfo, log};
    web.start();

    xTaskCreate(dns_task, "dns_task", 2048, &dns, 1, nullptr);

    // Status LED: solid-blinking blue while the captive portal is up.
    s_led.setStatus(gh::app::SystemStatus::Provisioning);
    xTaskCreate(led_tick_task, "led_task",
                gh::coord::CoordinatorConfig::kLedTaskStackBytes,
                &s_led,
                gh::coord::CoordinatorConfig::kLedTaskPriority,
                nullptr);
    log.info("provisioning", "AP up, web server listening");
}

}  // namespace

void setup() {
    Serial.begin(115200);
    static gh::infra::SerialLogger log{};

    // Explicit partition label: partitions.csv names the FS partition
    // "littlefs", but LittleFS.begin() defaults to looking for "spiffs" and
    // would silently fail to mount, leaving the web SPA unserved.
    if (!LittleFS.begin(/*formatOnFail=*/true, "/littlefs", /*maxOpenFiles=*/10,
                        /*partitionLabel=*/"littlefs")) {
        Serial.println("WARN: LittleFS mount failed");
    }

    char device_id_buf[8] = {};
    {
        uint8_t mac[6] = {0};
        // esp_read_mac() reads the base MAC from eFuse and works before the
        // Wi-Fi stack is up — unlike WiFi.macAddress(), which returned all-zeros
        // here and collapsed every device's id/SSID to "000000".
        esp_read_mac(mac, ESP_MAC_WIFI_STA);
        std::snprintf(device_id_buf, sizeof(device_id_buf),
                      "%02x%02x%02x", mac[3], mac[4], mac[5]);
    }
    // MUST be static: its .c_str() is stored by RestStatusRoutes / TaskCtx /
    // MQTT/HA wiring and read by tasks that outlive setup(). A plain local would
    // be destroyed when setup() returns, leaving those pointers dangling
    // (observed as device_id="" in /api/status, empty MQTT topics, etc.).
    static const std::string device_id{device_id_buf};
    Serial.printf("[coordinator] device_id=greenhouse_%s\n", device_id.c_str());

    static gh::infra::NvsWifiCredsStore wifiStore{};
    static gh::infra::NvsMqttCredsStore mqttStore{};
    static gh::infra::NvsSoilCalibrationStore soilStore{};
    static gh::infra::NvsProvisioningFlagStore prov_flag_store{};
    static gh::infra::NvsAdminCredsStore admin_creds_store{};
    static gh::infra::NvsAnalyticsConfigStore analytics_store{};
    static gh::infra::ArduinoSystemInfo sysinfo{kFirmwareVersion};

    if (wifiStore.begin() != gh::domain::ErrorCode::Ok) log.warn("nvs", "wifi open failed");
    if (mqttStore.begin() != gh::domain::ErrorCode::Ok) log.warn("nvs", "mqtt open failed");
    if (soilStore.begin() != gh::domain::ErrorCode::Ok) log.warn("nvs", "soil open failed");

    static gh::infra::GpioButton button{cfg::kBootButtonGpio};
    static gh::infra::NvsWifiFailCounterStore   wifi_fail_counter{};
    static gh::infra::NvsLastConnectErrorStore  wifi_last_error{};
    static gh::infra::WifiStaAdapter sta{wifi_last_error, wifi_fail_counter, log};
    sta.begin();

    static gh::app::WifiProvisioner provisioner{
        wifiStore, button, sta, prov_flag_store, wifi_fail_counter, log,
        cfg::kStaConnectTimeoutMs, cfg::kStaRetryCount, cfg::kProvisioningButtonHoldMs};

    // Amber while bootstrap() attempts the STA connect (a blocking busy-wait,
    // so this is a static colour, not animated). runProvisioning() overrides it
    // to blue if we end up in setup mode; led_task takes over when operational.
    s_led.setStatus(gh::app::SystemStatus::WifiConnecting);
    s_led.tick(millis());

    const auto mode = provisioner.bootstrap();
    if (mode == gh::app::SystemMode::Provisioning) {
        runProvisioning(log, wifiStore, mqttStore, soilStore, prov_flag_store,
                        wifi_last_error, admin_creds_store, analytics_store,
                        sysinfo);
    } else {
        runOperational(log, soilStore, mqttStore, wifiStore, prov_flag_store,
                       admin_creds_store, analytics_store, sysinfo, device_id);
    }
}

void loop() {
    // Empty; both branches deliver via tasks.
}
