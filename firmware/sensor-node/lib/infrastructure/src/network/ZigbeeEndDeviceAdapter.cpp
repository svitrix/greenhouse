// NOLINTBEGIN(*-macro-usage)
// Suppress lint on esp-idf C macros that we cannot modify.

#include "ZigbeeEndDeviceAdapter.hpp"
#include "ZclIds.hpp"
#include "ZigbeeNetwork.hpp"
#include "secrets.hpp"

// esp-zigbee-sdk and Preferences.h pull in Arduino-ESP32 headers
// (WString.h, etc.) that have benign -Wconversion issues we cannot fix
// upstream. Suppress only around these SDK includes.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
// esp-zigbee-sdk is bundled in framework-arduinoespressif32-libs and linked
// automatically by pioarduino-build.py for esp32c6.  No extra lib_dep needed.
#include <esp_zigbee_core.h>
#include <esp_zigbee_cluster.h>
#include <esp_zigbee_endpoint.h>
#include <esp_zigbee_secur.h>
#include "aps/esp_zigbee_aps.h"
#include "nwk/esp_zigbee_nwk.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <Preferences.h>
#pragma GCC diagnostic pop

#include <atomic>
#include <cstdio>
#include <cstring>
#ifdef ARDUINO
#include <Arduino.h>
#endif

namespace gh::infra {

// ---------------------------------------------------------------------------
// Module-level state (kept static so no heap allocation after setup())
// ---------------------------------------------------------------------------
namespace {

// Counts APS confirms (per cycle) addressed to the coordinator.
// Reset by reportAttribute() before issuing a report. Updated by on_aps_data_confirm.
// Acquire/release ordering pairs with the consumer loop in reportAttribute().
static std::atomic<uint8_t> s_confirms_received{0};  // NOLINT(*-avoid-non-const-global)
static std::atomic<uint8_t> s_confirms_failed{0};    // NOLINT(*-avoid-non-const-global)

// std::atomic so writes from the Zigbee task are visible to start()'s poll
// loop in setup() without UB.  `volatile` is not enough — it doesn't establish
// a happens-before edge, so the read of s_actual_tc_ieee could be reordered
// across the read of s_joined on a compiler that reorders ops aggressively.
std::atomic<bool> s_joined{false};        // NOLINT(*-avoid-non-const-global)
std::atomic<bool> s_tc_mismatch{false};   // NOLINT(*-avoid-non-const-global)

// True if NVS already holds a saved Trust Center IEEE (i.e. we have paired
// at least once before). Loaded from NVS at the top of start().
bool s_has_saved_tc = false;              // NOLINT(*-avoid-non-const-global)

// Saved Trust Center IEEE from NVS (only meaningful when s_has_saved_tc).
uint8_t s_saved_tc_ieee[8] = {0};         // NOLINT(*-avoid-non-const-global)

// Actual Trust Center IEEE observed after join — captured in the signal
// handler, persisted by start() after a successful first pair.
// Written BEFORE s_joined.store(true) in confirm_join_with_tc_verification
// so the start() task always sees a consistent IEEE after seeing s_joined.
uint8_t s_actual_tc_ieee[8] = {0};        // NOLINT(*-avoid-non-const-global)

// Initial-value storage for ZCL attribute registration.  The SDK stores a pointer
// to these at esp_zb_device_register() time; they must outlive the stack frame.
// Values are used as initial attribute defaults; reportAttribute() updates them
// via esp_zb_zcl_set_attribute_val before issuing a report.
uint16_t s_soil_zcl              = 0;   // uint16, centi-% (raw-scaled)    NOLINT
uint8_t  s_battery_pct_zcl       = 0;   // uint8,  half-percentage units   NOLINT
uint8_t  s_battery_voltage_zcl   = 0;   // uint8,  units of 100 mV (ZCL)   NOLINT
uint32_t s_report_period_s       = gh::protocol::kReportPeriodDefaultS;  // NOLINT
// Backing store for manufacturer-specific Basic attribute 0xF001.
// Bit n is set when SensorChannelId{n} reported Ok this cycle.
// Updated via esp_zb_zcl_set_attribute_val() before each report.
uint32_t s_sensors_present_mask  = 0;   // uint32, bitmask, read-only      NOLINT

// Bounded lock timeout — if the Zigbee task is wedged we'd rather bail and
// deep-sleep than block the active-power loop forever.  500 ms is generous
// for a healthy stack (sub-ms typically).
constexpr TickType_t kZbLockTimeoutTicks = pdMS_TO_TICKS(500);

// ZCL spec: attribute IDs 0xF000..0xFFFE are reserved for manufacturer-
// specific attributes (require ManufacturerCode field in the frame header).
constexpr uint16_t kZclMfgSpecificAttrBase = 0xF000U;

// ---------------------------------------------------------------------------
// Trust-Center verification — runs on every successful join/rejoin.
// Sets s_joined on success, s_tc_mismatch on a different TC than what we
// paired with on the very first boot.
// ---------------------------------------------------------------------------
void confirm_join_with_tc_verification() noexcept {
    esp_zb_aps_get_trust_center_address(s_actual_tc_ieee);

    // Sanity: a TC of all-zero means the API call landed before the stack
    // had finished writing it. Treat as transient failure — start() will
    // wait out its timeout and the next wake will retry.
    bool all_zero = true;
    for (uint8_t i = 0; i < 8; ++i) {
        if (s_actual_tc_ieee[i] != 0) { all_zero = false; break; }
    }
    if (all_zero) {
        return;
    }

    if (s_has_saved_tc) {
        if (memcmp(s_actual_tc_ieee, s_saved_tc_ieee, 8) == 0) {
            // Write IEEE (above) happens-before the store-release on s_joined,
            // so start()'s acquire-load on s_joined sees a consistent IEEE.
            s_joined.store(true, std::memory_order_release);
        } else {
            s_tc_mismatch.store(true, std::memory_order_release);
        }
    } else {
        // First-ever pair — accept this TC. start() will persist it to
        // NVS once it sees s_joined == true.
        s_joined.store(true, std::memory_order_release);
    }
}

// ---------------------------------------------------------------------------
// APS data-confirm callback — fires from the Zigbee main-loop task once an
// outgoing frame is either ack'd by the parent or finally fails.  Increments
// either s_confirms_received or s_confirms_failed so reportAttribute()'s poll
// loop can wait for the confirm (or time out).
// ---------------------------------------------------------------------------
void on_aps_data_confirm(esp_zb_apsde_data_confirm_t confirm) {
    constexpr uint16_t kCoordShortAddr = 0x0000U;
    if (confirm.dst_addr.addr_short != kCoordShortAddr) {
        return;
    }
    if (confirm.status == 0) {
        s_confirms_received.fetch_add(1, std::memory_order_release);
    } else {
        s_confirms_failed.fetch_add(1, std::memory_order_release);
    }
}

// ---------------------------------------------------------------------------
// Zigbee app signal handler — required symbol, called from Zigbee task.
// esp_zb_app_signal_handler has C linkage and is a weak symbol in the SDK;
// we must define it exactly once per firmware binary.
// ---------------------------------------------------------------------------
extern "C" void esp_zb_app_signal_handler(esp_zb_app_signal_t* signal_struct) {
    const auto sig_type = static_cast<esp_zb_app_signal_type_t>(
        *signal_struct->p_app_signal);
    const esp_err_t status = signal_struct->esp_err_status;

    switch (sig_type) {
    case ESP_ZB_ZDO_SIGNAL_SKIP_STARTUP:
        // Stack initialised — begin network steering.
        esp_zb_bdb_start_top_level_commissioning(
            ESP_ZB_BDB_MODE_NETWORK_STEERING);
        break;

    case ESP_ZB_BDB_SIGNAL_DEVICE_FIRST_START:
        // Factory-new boot, no saved network — start steering.
        esp_zb_bdb_start_top_level_commissioning(
            ESP_ZB_BDB_MODE_NETWORK_STEERING);
        break;

    case ESP_ZB_BDB_SIGNAL_DEVICE_REBOOT:
        if (status == ESP_OK) {
            // Restored saved network from NVS — verify the TC IEEE matches
            // the one we paired with before flagging join complete.
            confirm_join_with_tc_verification();
        } else {
            // Saved network gone / corrupt — fall back to fresh steering.
            esp_zb_bdb_start_top_level_commissioning(
                ESP_ZB_BDB_MODE_NETWORK_STEERING);
        }
        break;

    case ESP_ZB_BDB_SIGNAL_STEERING:
        if (status == ESP_OK) {
            confirm_join_with_tc_verification();
        }
        // If status != ESP_OK, steering timed out — start() will detect
        // via s_joined still being false.
        break;

    default:
        break;
    }
}

// ---------------------------------------------------------------------------
// Zigbee main-loop task — runs esp_zb_stack_main_loop() forever.
// Stack size 8192 BYTES (ESP-IDF FreeRTOS port: usStackDepth is bytes, not
// words like vanilla FreeRTOS).  Espressif's own examples use 8 KB for the
// Zigbee task; 4 KB is borderline and risks a silent stack overflow.
// Priority: just above idle so application tasks can preempt.
// ---------------------------------------------------------------------------
constexpr uint32_t kZbTaskStackBytes = 8192U;

void zb_task(void* /*arg*/) {
    // esp_zb_start failure here is fatal for this boot — but we don't want
    // to abort() (battery device).  Mark "not joined", and the start() task
    // will time out and route us to deep-sleep retry.
    if (esp_zb_start(false) != ESP_OK) {
        vTaskDelete(nullptr);
        return;
    }
    esp_zb_stack_main_loop();
    vTaskDelete(nullptr);
}

// ---------------------------------------------------------------------------
// Endpoint registration helpers
// ---------------------------------------------------------------------------

// Manufacturer-name string in ZCL format: first byte = length.
// Static storage — must outlive esp_zb_device_register().
constexpr uint8_t kManufNameLen =
    static_cast<uint8_t>(sizeof(gh::protocol::kBasicManufacturer) - 1U);
uint8_t s_manuf_name[sizeof(gh::protocol::kBasicManufacturer) + 1U] = {};

constexpr uint8_t kModelIdLen =
    static_cast<uint8_t>(sizeof(gh::protocol::kBasicModelSensorNode) - 1U);
uint8_t s_model_id[sizeof(gh::protocol::kBasicModelSensorNode) + 1U] = {};

// Stores the last ZCL registration failure for diagnostic heartbeats.
// Written by ZB_RETURN_IF_FAIL, read by initErrContext().
static char s_zb_init_err[96] = "none";  // NOLINT(*-avoid-non-const-global)

// Helper: every SDK call that returns esp_err_t routes failures into a
// graceful ErrorCode return rather than abort().  ESP_ERROR_CHECK would
// brick the node on any transient stack quirk — fatal for a battery device.
// Stores the failing call + errno string so the no-sleep heartbeat can
// print it every iteration regardless of USB-CDC enumeration timing.
#define ZB_RETURN_IF_FAIL(call)                                                \
    do {                                                                       \
        const esp_err_t _zb_err = (call);                                      \
        if (_zb_err != ESP_OK) {                                               \
            snprintf(s_zb_init_err, sizeof(s_zb_init_err),                     \
                     "%.55s: %s", #call, esp_err_to_name(_zb_err));            \
            log_.error("DIAG", s_zb_init_err);                                 \
            return gh::domain::ErrorCode::ZigbeeStackInitFailed;               \
        }                                                                      \
    } while (0)

}  // namespace

// ---------------------------------------------------------------------------
// Static accessor — safe to call from any context after start() is invoked.
// ---------------------------------------------------------------------------
const char* ZigbeeEndDeviceAdapter::initErrContext() noexcept {
    return s_zb_init_err;
}

// ---------------------------------------------------------------------------
// ZigbeeEndDeviceAdapter implementation
// ---------------------------------------------------------------------------

ZigbeeEndDeviceAdapter::ZigbeeEndDeviceAdapter(gh::domain::ILogger& log,
                                               gh::domain::IRgbLed& pairing_led) noexcept
    : log_(log), pairing_led_(pairing_led) {
    // Prepare ZCL length-prefixed strings.
    s_manuf_name[0] = kManufNameLen;
    for (uint8_t i = 0; i < kManufNameLen; ++i) {
        s_manuf_name[1U + i] = static_cast<uint8_t>(
            gh::protocol::kBasicManufacturer[i]);
    }

    s_model_id[0] = kModelIdLen;
    for (uint8_t i = 0; i < kModelIdLen; ++i) {
        s_model_id[1U + i] = static_cast<uint8_t>(
            gh::protocol::kBasicModelSensorNode[i]);
    }

}

void ZigbeeEndDeviceAdapter::clearPairingNvs() noexcept {
    Preferences prefs;
    if (prefs.begin("zigbee_pair", /*readOnly=*/false)) {
        prefs.clear();
        prefs.end();
    }
}

void ZigbeeEndDeviceAdapter::pairingLedOff_() noexcept {
    if (pairing_led_on_) {
        pairing_led_.setColor(0, 0, 0);
        pairing_led_on_ = false;
    }
}

void ZigbeeEndDeviceAdapter::tickPairingLed_(uint32_t now_ms) noexcept {
    const bool on =
        (now_ms % gh::protocol::kPairingLedPeriodMs) < gh::protocol::kPairingLedOnMs;
    if (on == pairing_led_on_) {
        return;
    }
    if (on) {
        pairing_led_.setColor(gh::protocol::kPairingLedR,
                              gh::protocol::kPairingLedG,
                              gh::protocol::kPairingLedB);
    } else {
        pairing_led_.setColor(0, 0, 0);
    }
    pairing_led_on_ = on;
}

gh::domain::ErrorCode
ZigbeeEndDeviceAdapter::buildAndRegisterEndpoints_() noexcept {
    // 3. Build endpoint 1 cluster list.
    esp_zb_cluster_list_t* cluster_list = esp_zb_zcl_cluster_list_create();

    // --- Basic cluster (server) ---
    {
        esp_zb_basic_cluster_cfg_t basic_cfg{};
        basic_cfg.zcl_version  = ESP_ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE;
        basic_cfg.power_source = ESP_ZB_ZCL_BASIC_POWER_SOURCE_BATTERY;

        esp_zb_attribute_list_t* basic_attrs =
            esp_zb_basic_cluster_create(&basic_cfg);

        ZB_RETURN_IF_FAIL(esp_zb_basic_cluster_add_attr(
            basic_attrs,
            ESP_ZB_ZCL_ATTR_BASIC_MANUFACTURER_NAME_ID,
            s_manuf_name));

        ZB_RETURN_IF_FAIL(esp_zb_basic_cluster_add_attr(
            basic_attrs,
            ESP_ZB_ZCL_ATTR_BASIC_MODEL_IDENTIFIER_ID,
            s_model_id));

        ZB_RETURN_IF_FAIL(esp_zb_cluster_add_attr(
            basic_attrs,
            static_cast<uint16_t>(gh::protocol::kClusterBasic),
            gh::protocol::kAttrBasicReportPeriodS,
            ESP_ZB_ZCL_ATTR_TYPE_U32,
            ESP_ZB_ZCL_ATTR_ACCESS_READ_WRITE,
            &s_report_period_s));

        ZB_RETURN_IF_FAIL(esp_zb_cluster_add_attr(
            basic_attrs,
            static_cast<uint16_t>(gh::protocol::kClusterBasic),
            gh::protocol::kAttrBasicSensorsPresentMask,
            ESP_ZB_ZCL_ATTR_TYPE_U32,
            ESP_ZB_ZCL_ATTR_ACCESS_READ_ONLY,
            &s_sensors_present_mask));

        ZB_RETURN_IF_FAIL(esp_zb_cluster_list_add_basic_cluster(
            cluster_list, basic_attrs, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE));
    }
    log_.error("DIAG", "basic cluster ok");

    // --- Power Configuration cluster (server) ---
    {
        esp_zb_power_config_cluster_cfg_t power_cfg{};
        esp_zb_attribute_list_t* power_attrs =
            esp_zb_power_config_cluster_create(&power_cfg);

        ZB_RETURN_IF_FAIL(esp_zb_power_config_cluster_add_attr(
            power_attrs,
            gh::protocol::kAttrBatteryVoltage,
            &s_battery_voltage_zcl));

        ZB_RETURN_IF_FAIL(esp_zb_power_config_cluster_add_attr(
            power_attrs,
            gh::protocol::kAttrBatteryPercentageRemaining,
            &s_battery_pct_zcl));

        ZB_RETURN_IF_FAIL(esp_zb_cluster_list_add_power_config_cluster(
            cluster_list, power_attrs, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE));
    }

    // --- Temperature Measurement cluster (server) ---
    {
        esp_zb_temperature_meas_cluster_cfg_t temp_cfg{};
        temp_cfg.measured_value =
            ESP_ZB_ZCL_TEMP_MEASUREMENT_MEASURED_VALUE_DEFAULT;
        temp_cfg.min_value =
            ESP_ZB_ZCL_TEMP_MEASUREMENT_MIN_MEASURED_VALUE_DEFAULT;
        temp_cfg.max_value =
            ESP_ZB_ZCL_TEMP_MEASUREMENT_MAX_MEASURED_VALUE_DEFAULT;

        ZB_RETURN_IF_FAIL(esp_zb_cluster_list_add_temperature_meas_cluster(
            cluster_list,
            esp_zb_temperature_meas_cluster_create(&temp_cfg),
            ESP_ZB_ZCL_CLUSTER_SERVER_ROLE));
    }

    // --- Relative Humidity cluster (server) ---
    {
        esp_zb_humidity_meas_cluster_cfg_t hum_cfg{};
        hum_cfg.measured_value = 0U;
        hum_cfg.min_value      = 0U;
        hum_cfg.max_value      = 10000U;

        ZB_RETURN_IF_FAIL(esp_zb_cluster_list_add_humidity_meas_cluster(
            cluster_list,
            esp_zb_humidity_meas_cluster_create(&hum_cfg),
            ESP_ZB_ZCL_CLUSTER_SERVER_ROLE));
    }

    // --- Soil moisture cluster (manufacturer ID — see kClusterSoilMoisture) ---
    {
        esp_zb_attribute_list_t* soil_attrs =
            esp_zb_zcl_attr_list_create(
                static_cast<uint16_t>(gh::protocol::kClusterSoilMoisture));

        ZB_RETURN_IF_FAIL(esp_zb_cluster_add_attr(
            soil_attrs,
            static_cast<uint16_t>(gh::protocol::kClusterSoilMoisture),
            gh::protocol::kAttrMeasuredValue,
            ESP_ZB_ZCL_ATTR_TYPE_U16,
            ESP_ZB_ZCL_ATTR_ACCESS_READ_ONLY,
            &s_soil_zcl));

        ZB_RETURN_IF_FAIL(esp_zb_cluster_list_add_custom_cluster(
            cluster_list, soil_attrs, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE));
    }
    log_.error("DIAG", "all clusters ok");

    // 4. Register endpoints.
    esp_zb_ep_list_t* ep_list = esp_zb_ep_list_create();

    {
        esp_zb_endpoint_config_t ep_cfg{};
        ep_cfg.endpoint           = gh::protocol::kSensorEndpoint;
        ep_cfg.app_profile_id     = ESP_ZB_AF_HA_PROFILE_ID;
        ep_cfg.app_device_id      = ESP_ZB_HA_TEMPERATURE_SENSOR_DEVICE_ID;
        ep_cfg.app_device_version = 0U;
        ZB_RETURN_IF_FAIL(esp_zb_ep_list_add_ep(ep_list, cluster_list, ep_cfg));
    }

    {
        esp_zb_temperature_meas_cluster_cfg_t soil_temp_cfg{};
        soil_temp_cfg.measured_value = ESP_ZB_ZCL_TEMP_MEASUREMENT_MEASURED_VALUE_DEFAULT;
        soil_temp_cfg.min_value      = -4000;
        soil_temp_cfg.max_value      =  8500;

        esp_zb_cluster_list_t* ep2_clusters = esp_zb_zcl_cluster_list_create();
        ZB_RETURN_IF_FAIL(esp_zb_cluster_list_add_temperature_meas_cluster(
            ep2_clusters,
            esp_zb_temperature_meas_cluster_create(&soil_temp_cfg),
            ESP_ZB_ZCL_CLUSTER_SERVER_ROLE));

        esp_zb_endpoint_config_t ep2_cfg{};
        ep2_cfg.endpoint           = gh::protocol::kSensorSoilTempEndpoint;
        ep2_cfg.app_profile_id     = ESP_ZB_AF_HA_PROFILE_ID;
        ep2_cfg.app_device_id      = ESP_ZB_HA_TEMPERATURE_SENSOR_DEVICE_ID;
        ep2_cfg.app_device_version = 0U;
        ZB_RETURN_IF_FAIL(esp_zb_ep_list_add_ep(ep_list, ep2_clusters, ep2_cfg));
    }

    // 5. Register device.
    log_.error("DIAG", "before device_register");
    ZB_RETURN_IF_FAIL(esp_zb_device_register(ep_list));
    log_.error("DIAG", "device_register ok");

    esp_zb_set_node_descriptor_manufacturer_code(gh::protocol::kManufacturerCode);

    return gh::domain::ErrorCode::Ok;
}

gh::domain::ErrorCode
ZigbeeEndDeviceAdapter::start(uint32_t steering_timeout_ms) noexcept {
    static bool s_started = false;  // NOLINT(*-avoid-non-const-global)
    if (s_started) {
        log_.warn("zb-start", "start() called twice; second call ignored");
        return gh::domain::ErrorCode::Ok;
    }
    s_started = true;

    // 0. Load previously-paired Trust Center IEEE from NVS (if any).
    //    The signal handler will use this to verify we re-attach to the
    //    same coordinator on every wake.
    s_joined.store(false, std::memory_order_release);
    s_tc_mismatch.store(false, std::memory_order_release);
    s_has_saved_tc = false;
    {
        Preferences prefs;
        if (prefs.begin("zigbee_pair", /*readOnly=*/true)) {
            const size_t got = prefs.getBytes(
                "tc_ieee", s_saved_tc_ieee, sizeof(s_saved_tc_ieee));
            s_has_saved_tc = (got == sizeof(s_saved_tc_ieee));
            prefs.end();
            if (!s_has_saved_tc) {
                log_.info("zb-pair",
                          "no saved trust center, fresh pair on this boot");
            }
        } else {
            log_.info("zb-pair",
                      "nvs namespace empty, fresh pair on this boot");
        }
    }

    // 1. Platform config — native 802.15.4 radio, no host connection.
    esp_zb_platform_config_t platform_config{};
    platform_config.radio_config.radio_mode      = ZB_RADIO_MODE_NATIVE;
    platform_config.host_config.host_connection_mode =
        ZB_HOST_CONNECTION_MODE_NONE;
    if (esp_zb_platform_config(&platform_config) != ESP_OK) {
        log_.error("DIAG", "platform_config FAILED");   // DIAG
        return gh::domain::ErrorCode::ZigbeeStackInitFailed;
    }
    log_.error("DIAG", "platform ok");                  // DIAG (error level = visible)

    // 2. Zigbee stack config — sleepy End Device.
    esp_zb_cfg_t zb_cfg{};
    zb_cfg.esp_zb_role          = ESP_ZB_DEVICE_TYPE_ED;
    zb_cfg.install_code_policy  = false;
    zb_cfg.nwk_cfg.zed_cfg.ed_timeout  = ESP_ZB_ED_AGING_TIMEOUT_64MIN;
    // Keep-alive: 3000 ms (poll parent when radio is on).
    zb_cfg.nwk_cfg.zed_cfg.keep_alive  = 3000U;
    esp_zb_init(&zb_cfg);

    // 2.1. Register APS data-confirm so reportAttribute()'s poll loop actually
    //      gets signalled when our frame is ack'd by the parent.  Must be
    //      registered AFTER esp_zb_init and BEFORE esp_zb_start.
    esp_zb_aps_data_confirm_handler_register(&on_aps_data_confirm);

    // 2.2. Network identity — pin channel mask only. The ExtPanId is NOT
    //      asserted here: the coordinator generates and persists its own
    //      random ExtPanId on first boot (NVS namespace "zigbee_net").
    //      The sensor-node attaches by TC link key + channel mask and
    //      accepts whatever ExtPanId the network advertises.
    esp_zb_set_primary_network_channel_set(gh::protocol::kZigbeeChannelMask);

    // 2.3. Security — custom Trust Center pre-configured link key (replaces
    //      the well-known ZigbeeAlliance09 default), and require APS
    //      link-key exchange so the TC key is used only for the initial
    //      handshake — all subsequent traffic uses a derived per-link key.
    // SDK signature is non-const but does not modify the buffer.
    esp_zb_secur_TC_standard_preconfigure_key_set(
        const_cast<uint8_t*>(gh::protocol::kZigbeeTcLinkKey));
    esp_zb_secur_link_key_exchange_required_set(true);

    // 3–5. Build clusters, register endpoints, call esp_zb_device_register().
    //      Extracted to a helper so start() can always launch zb_task even on
    //      partial failure — leaving esp_zb_init() unstarted causes MAC/PHY
    //      internal tasks to panic ~10 s later.
    const auto reg_result = buildAndRegisterEndpoints_();

    // 6. Launch Zigbee task unconditionally — prevents MAC crash on failure.
    xTaskCreate(zb_task,
                "zb_task",
                kZbTaskStackBytes,   // ESP-IDF FreeRTOS: bytes, not words
                nullptr,
                tskIDLE_PRIORITY + 1U,
                nullptr);

    if (reg_result != gh::domain::ErrorCode::Ok) {
        return reg_result;
    }

    // 7. Wait for join, mismatch, or timeout.
    {
        char b[40];
        std::snprintf(b, sizeof(b), "steering, timeout=%ums",
                      static_cast<unsigned>(steering_timeout_ms));
        log_.error("DIAG", b);   // DIAG (error = visible over USB-CDC)
    }
    constexpr uint32_t kStepMs = 500U;
    uint32_t waited_ms = 0U;
    while (!s_joined.load(std::memory_order_acquire)
        && !s_tc_mismatch.load(std::memory_order_acquire)
        && waited_ms < steering_timeout_ms) {
        tickPairingLed_(millis());
        vTaskDelay(pdMS_TO_TICKS(kStepMs));
        waited_ms += kStepMs;
    }
    pairingLedOff_();

    if (s_tc_mismatch.load(std::memory_order_acquire)) {
        return gh::domain::ErrorCode::ZigbeeTrustCenterMismatch;
    }
    if (!s_joined.load(std::memory_order_acquire)) {
        char b[48];
        std::snprintf(b, sizeof(b), "no join after %ums",
                      static_cast<unsigned>(waited_ms));
        log_.warn("zb-start", b);   // DIAG
        return gh::domain::ErrorCode::ZigbeeJoinTimeout;
    }

    // 8. First-ever pair — persist the observed Trust Center IEEE so we
    //    can verify it matches on every subsequent boot.
    if (!s_has_saved_tc) {
        Preferences prefs;
        if (prefs.begin("zigbee_pair", /*readOnly=*/false)) {
            const size_t put = prefs.putBytes(
                "tc_ieee", s_actual_tc_ieee, sizeof(s_actual_tc_ieee));
            prefs.end();
            if (put != sizeof(s_actual_tc_ieee)) {
                log_.warn("zb-pair",
                          "nvs write short, tc-mismatch check will be skipped next boot");
            }
        } else {
            log_.warn("zb-pair",
                      "nvs open for write failed, no tc persistence");
        }
    }

    return gh::domain::ErrorCode::Ok;
}

gh::domain::ErrorCode
ZigbeeEndDeviceAdapter::reportAttribute(uint8_t  endpoint,
                                         uint16_t cluster_id,
                                         uint16_t attribute_id,
                                         gh::domain::ZclType type,
                                         const void* data,
                                         size_t   size,
                                         uint32_t tx_timeout_ms) noexcept {
    // `size` is intentionally unused: esp_zb_zcl_set_attribute_val() looks up
    // the attribute's size from the registered schema, not from the caller.
    // Kept in the signature so callers don't have to track this asymmetry.
    (void)size;
    // NOT re-entrant: only one reportAttribute call may be in flight at a
    // time. SensorCycle is single-threaded so this holds today. Resetting here
    // discards any stale confirms from a prior timed-out call.
    s_confirms_received.store(0, std::memory_order_release);
    s_confirms_failed.store(0, std::memory_order_release);

    // 1:1 cast — ZclType values were chosen to match ESP_ZB_ZCL_ATTR_TYPE_*.
    const auto zcl_type = static_cast<esp_zb_zcl_attr_type_t>(
        static_cast<uint8_t>(type));

    if (!esp_zb_lock_acquire(kZbLockTimeoutTicks)) {
        return gh::domain::ErrorCode::NetworkDown;
    }

    // Write the attribute value into the local ZCL attribute store so the
    // SDK reads the current value when building the Report Attributes frame.
    // data pointer ownership stays with the caller; set_attribute_val copies.
    esp_zb_zcl_set_attribute_val(
        endpoint,
        cluster_id,
        ESP_ZB_ZCL_CLUSTER_SERVER_ROLE,
        attribute_id,
        // SDK signature is non-const but does not modify the buffer.
        const_cast<void*>(data),  // NOLINT(*-const-cast)
        false);

    constexpr uint16_t kCoordAddr = 0x0000U;
    constexpr uint8_t  kCoordEp   = 1U;

    // Manufacturer-specific attributes occupy the 0xF000..0xFFFE range.
    const bool is_manuf_specific = (attribute_id >= kZclMfgSpecificAttrBase);

    esp_zb_zcl_report_attr_cmd_t cmd{};
    cmd.zcl_basic_cmd.dst_addr_u.addr_short = kCoordAddr;
    cmd.zcl_basic_cmd.dst_endpoint          = kCoordEp;
    cmd.zcl_basic_cmd.src_endpoint          = endpoint;
    cmd.address_mode                        = ESP_ZB_APS_ADDR_MODE_16_ENDP_PRESENT;
    cmd.direction                           = ESP_ZB_ZCL_CMD_DIRECTION_TO_CLI;
    cmd.manuf_specific                      = is_manuf_specific ? 1U : 0U;
    cmd.manuf_code                          = is_manuf_specific
                                                  ? gh::protocol::kManufacturerCode
                                                  : 0U;
    cmd.dis_default_resp                    = 0U;
    cmd.clusterID                           = cluster_id;
    cmd.attributeID                         = attribute_id;

    // Suppress unused-variable warning: zcl_type is verified at compile time
    // via the static_assert below; the SDK's set_attribute_val already knows
    // the type from the attribute registration and does not take a type arg.
    (void)zcl_type;

    esp_zb_zcl_report_attr_cmd_req(&cmd);

    esp_zb_lock_release();

    // 2. Poll until exactly 1 APS confirm arrives or timeout.
    constexpr uint32_t kPollIntervalMs  = 20U;
    constexpr uint8_t  kExpectedConfirms = 1U;
    const uint32_t deadline_ms = millis() + tx_timeout_ms;
    while (millis() < deadline_ms) {
        const uint8_t rcvd = s_confirms_received.load(std::memory_order_acquire);
        const uint8_t fail = s_confirms_failed.load(std::memory_order_acquire);
        if (static_cast<unsigned>(rcvd) + fail >= kExpectedConfirms) {
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(kPollIntervalMs));
    }

    const uint8_t rcvd = s_confirms_received.load(std::memory_order_acquire);
    if (rcvd == kExpectedConfirms) {
        return gh::domain::ErrorCode::Ok;
    }

    const uint8_t fail = s_confirms_failed.load(std::memory_order_acquire);
    char warn_buf[64];
    (void)snprintf(warn_buf, sizeof(warn_buf),
                   "reportAttr tx incomplete: ok=%d fail=%d cid=0x%04x aid=0x%04x",
                   static_cast<int>(rcvd),
                   static_cast<int>(fail),
                   static_cast<unsigned>(cluster_id),
                   static_cast<unsigned>(attribute_id));
    log_.warn("zb-report", warn_buf);
    return gh::domain::ErrorCode::MqttDisconnected;
}

// Compile-time verification that ZclType values match the SDK enum.
// If the SDK ever changes these values the static_assert will catch it
// at build time rather than silently producing wrong-type ZCL frames.
static_assert(static_cast<uint8_t>(gh::domain::ZclType::Uint8)  == ESP_ZB_ZCL_ATTR_TYPE_U8,
              "ZclType::Uint8 must match ESP_ZB_ZCL_ATTR_TYPE_U8");
static_assert(static_cast<uint8_t>(gh::domain::ZclType::Uint16) == ESP_ZB_ZCL_ATTR_TYPE_U16,
              "ZclType::Uint16 must match ESP_ZB_ZCL_ATTR_TYPE_U16");
static_assert(static_cast<uint8_t>(gh::domain::ZclType::Uint32) == ESP_ZB_ZCL_ATTR_TYPE_U32,
              "ZclType::Uint32 must match ESP_ZB_ZCL_ATTR_TYPE_U32");
static_assert(static_cast<uint8_t>(gh::domain::ZclType::Int16)  == ESP_ZB_ZCL_ATTR_TYPE_S16,
              "ZclType::Int16 must match ESP_ZB_ZCL_ATTR_TYPE_S16");

uint32_t ZigbeeEndDeviceAdapter::reportPeriodSeconds() const noexcept {
    uint32_t period_s = gh::protocol::kReportPeriodDefaultS;
    // Guard against a concurrent Write Attribute from the coordinator —
    // esp_zb_zcl_get_attribute returns a pointer that the writer can
    // mutate underfoot. The lock pairs with the SDK's internal serialisation.
    if (esp_zb_lock_acquire(kZbLockTimeoutTicks)) {
        const esp_zb_zcl_attr_t* attr = esp_zb_zcl_get_attribute(
            gh::protocol::kSensorEndpoint,
            static_cast<uint16_t>(gh::protocol::kClusterBasic),
            ESP_ZB_ZCL_CLUSTER_SERVER_ROLE,
            gh::protocol::kAttrBasicReportPeriodS);
        if (attr != nullptr && attr->data_p != nullptr) {
            // memcpy avoids misaligned 32-bit load on RISC-V.
            std::memcpy(&period_s, attr->data_p, sizeof(period_s));
        }
        esp_zb_lock_release();
    }
    if (period_s < gh::protocol::kReportPeriodMinS) period_s = gh::protocol::kReportPeriodMinS;
    if (period_s > gh::protocol::kReportPeriodMaxS) period_s = gh::protocol::kReportPeriodMaxS;
    return period_s;
}

}  // namespace gh::infra

// NOLINTEND(*-macro-usage)
