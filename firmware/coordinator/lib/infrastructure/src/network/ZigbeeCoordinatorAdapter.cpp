// NOLINTBEGIN(*-macro-usage)
// Suppress lint on esp-idf C macros that we cannot modify.

#include "ZigbeeCoordinatorAdapter.hpp"
#include "ZclIds.hpp"
#include "ZclSensorMapper.hpp"
#include "ProtoVersion.hpp"
#include "ZigbeeNetwork.hpp"
#include "ports/IZigbeeReportSink.hpp"
#include "entities/ChannelSample.hpp"
#include "secrets.hpp"

// esp-zigbee-sdk is bundled in framework-arduinoespressif32-libs and linked
// automatically by pioarduino-build.py for esp32c6.  No extra lib_dep needed.
#include <esp_zigbee_core.h>
#include <esp_zigbee_cluster.h>
#include <esp_zigbee_secur.h>
#include "nwk/esp_zigbee_nwk.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <Arduino.h>

#include <array>
#include <atomic>
#include <cstring>

namespace gh::infra {

// ---------------------------------------------------------------------------
// Module-level state (static — no heap allocation after setup())
// ---------------------------------------------------------------------------
namespace {

// Multi-node sink — wired by ZigbeeCoordinatorAdapter::setReportSink. All
// inbound channel + presence + ZDO events flow into it from the Zigbee task.
static gh::domain::IZigbeeReportSink* s_sink = nullptr;  // NOLINT

// Permit-join duration in seconds to open on network formation.
static uint32_t s_permit_join_s = 0U;  // NOLINT

// millis() timestamp when the current permit-join window closes. Zero means
// closed. Set by recordPermitJoinOpen() from the Zigbee task or REST path.
static std::atomic<uint32_t> s_permit_join_until_ms{0};  // NOLINT

void recordPermitJoinOpen(uint8_t duration_s) noexcept {
    if (duration_s == 0) {
        return;
    }
    s_permit_join_until_ms.store(
        millis() + static_cast<uint32_t>(duration_s) * 1000U,
        std::memory_order_release);
}

// Per-coordinator ExtPanId — set from the composition root via the ctor
// (loaded from NVS or generated fresh on first boot).
static std::array<uint8_t, 8> s_ext_pan_id{};  // NOLINT(*-avoid-non-const-global)

// ---------------------------------------------------------------------------
// Known-sensor tracking — records short_addrs seen in reports and flags
// them for a one-time write of the desired report period (0xFF00).
// Drained by ZigbeeCoordinatorAdapter::drainPendingPeriodWrites().
// ---------------------------------------------------------------------------
// Must match gh::domain::kMaxRegisteredNodes (8): the registry/binding table
// support 8 nodes, so the period-write tracker must too — otherwise the 5th+
// sensor is silently skipped and never receives its desired report period.
constexpr uint8_t kMaxKnownNodes = 8;
struct KnownNode {
    uint16_t short_addr;
    bool     needs_period_write;
};
static std::array<KnownNode, kMaxKnownNodes> s_known_nodes{};  // NOLINT
static uint8_t s_known_count = 0;  // NOLINT

// Called from zb_action_handler on every report we receive. Records the
// sensor's short_addr and flags it as needing a one-time write of the
// desired report period. The composition root drains pending writes via
// drainPendingPeriodWrites() on each telemetry tick.
// Returns true the first time a given short_addr is seen this session, so the
// caller can run once-per-node work (e.g. (re)seed the binding).
static bool noteSensorReport(uint16_t short_addr) noexcept {
    for (uint8_t i = 0; i < s_known_count; ++i) {
        if (s_known_nodes[i].short_addr == short_addr) return false;
    }
    if (s_known_count >= kMaxKnownNodes) return false;
    s_known_nodes[s_known_count++] = { short_addr, true };
    return true;
}

// Convert an 8-byte little-endian IEEE address (esp_zb_ieee_addr_t) to a
// canonical uint64 (byte[0] = LSB). Used for both the in-band Device_annce
// address and the stack address-map lookup so they agree.
static uint64_t ieee_le_to_u64(const uint8_t* ieee_le) noexcept {
    uint64_t v = 0;
    for (int i = 7; i >= 0; --i) {
        v = (v << 8) | static_cast<uint8_t>(ieee_le[i]);
    }
    return v;
}

// ---------------------------------------------------------------------------
// Helper: read T from an unaligned const void* using memcpy.
// ---------------------------------------------------------------------------
template<typename T>
static T read_attr(const void* p) noexcept {
    T out{};
    memcpy(&out, p, sizeof(T));
    return out;
}

// ---------------------------------------------------------------------------
// ZCL action handler — called by the SDK for incoming ZCL frames.
// Each attribute emits independently into the IZigbeeReportSink:
//   - EP1 Basic 0xF001 (uint32 sensors_present_mask) → onPresenceFrame
//   - EP1 Basic 0xF002 (uint16 proto_version)        → onPresenceFrame
//   - Anything else known by ZclSensorMapper         → onChannelSample
// RSSI is reported as 0 because esp-zigbee-lib does not expose per-frame
// signal strength on the report-attr callback. Future hook (Espressif NWK
// indication) may surface it; until then NodeRegistry treats 0 as "unknown".
// ---------------------------------------------------------------------------
static esp_err_t zb_action_handler(
    esp_zb_core_action_callback_id_t callback_id,
    const void* message) noexcept
{
    if (s_sink == nullptr) return ESP_OK;
    if (callback_id != ESP_ZB_CORE_REPORT_ATTR_CB_ID) return ESP_OK;

    const auto* msg =
        static_cast<const esp_zb_zcl_report_attr_message_t*>(message);
    if (msg == nullptr || msg->attribute.data.value == nullptr) {
        return ESP_OK;
    }

    const uint16_t cluster  = msg->cluster;
    const uint16_t attr_id  = msg->attribute.id;
    const uint8_t  ep       = msg->src_endpoint;
    const uint16_t addr     = msg->src_address.u.short_addr;
    constexpr int8_t kRssiUnknown = 0;   // SDK does not expose per-frame RSSI.

    // Track sender so we can write the desired report period once. On first
    // sight this session, (re)seed the short_addr→IEEE binding from the stack's
    // address map. Without this, after a warm reboot (network restored from
    // NVS, in-RAM binding table empty) reports from an already-joined sleepy
    // end-device that never re-announces would be dropped by the router as
    // "unknown short_addr". The stack keeps the mapping in its NVRAM, so we
    // recover it here. Idempotent — onDeviceAnnounced de-dups by IEEE.
    if (noteSensorReport(addr)) {
        uint8_t ieee_le[8] = {};
        if (esp_zb_ieee_address_by_short(addr, ieee_le) == ESP_OK) {
            s_sink->onDeviceAnnounced(addr, ieee_le_to_u64(ieee_le));
        }
    }

    // EP1 Basic 0xF001 — sensors_present_mask (uint32)
    if (ep == gh::protocol::kSensorEndpoint &&
        cluster == gh::protocol::kClusterBasic &&
        attr_id == gh::protocol::kAttrBasicSensorsPresentMask &&
        msg->attribute.data.type == ESP_ZB_ZCL_ATTR_TYPE_U32 &&
        msg->attribute.data.size >= 4)
    {
        const uint32_t mask = read_attr<uint32_t>(msg->attribute.data.value);
        s_sink->onPresenceFrame(addr, mask, /*proto_version*/ 0, kRssiUnknown);
        return ESP_OK;
    }

    // EP1 Basic 0xF002 — proto_version (uint16, optional)
    if (ep == gh::protocol::kSensorEndpoint &&
        cluster == gh::protocol::kClusterBasic &&
        attr_id == gh::protocol::kAttrBasicProtoVersion &&
        msg->attribute.data.type == ESP_ZB_ZCL_ATTR_TYPE_U16 &&
        msg->attribute.data.size >= 2)
    {
        const uint16_t pv = read_attr<uint16_t>(msg->attribute.data.value);
        s_sink->onPresenceFrame(addr, /*mask*/ 0, pv, kRssiUnknown);
        return ESP_OK;
    }

    // Generic channel attribute via ZclSensorMapper.
    const auto dec = gh::infra::ZclSensorMapper::decode(
        ep, cluster, attr_id,
        static_cast<const uint8_t*>(msg->attribute.data.value),
        msg->attribute.data.size);
    if (dec.has_value()) {
        s_sink->onChannelSample(
            addr,
            gh::domain::ChannelSample{
                dec->kind,
                dec->quantity,
                dec->value_si,
                static_cast<uint32_t>(millis())
            },
            kRssiUnknown);
    }

    return ESP_OK;
}

// ---------------------------------------------------------------------------
// Zigbee app signal handler — required C-linkage weak symbol.
// Exactly one definition per firmware binary.
// ---------------------------------------------------------------------------
extern "C" void esp_zb_app_signal_handler(esp_zb_app_signal_t* signal_struct) {
    const auto sig_type = static_cast<esp_zb_app_signal_type_t>(
        *signal_struct->p_app_signal);
    const esp_err_t status = signal_struct->esp_err_status;

    switch (sig_type) {
    case ESP_ZB_ZDO_SIGNAL_SKIP_STARTUP:
        // Stack initialised — start top-level commissioning (network formation
        // for coordinator, or network steering for routers/end-devices).
        esp_zb_bdb_start_top_level_commissioning(
            ESP_ZB_BDB_MODE_NETWORK_FORMATION);
        break;

    case ESP_ZB_BDB_SIGNAL_FORMATION:
        if (status == ESP_OK) {
            // Brand-new network just formed → open the initial pairing
            // window so the sensor-node can join on its first wake.
            esp_zb_bdb_open_network(static_cast<uint8_t>(
                s_permit_join_s > 255U ? 255U : s_permit_join_s));
            recordPermitJoinOpen(static_cast<uint8_t>(
                s_permit_join_s > 255U ? 255U : s_permit_join_s));
        } else {
            esp_zb_bdb_start_top_level_commissioning(
                ESP_ZB_BDB_MODE_NETWORK_FORMATION);
        }
        break;

    case ESP_ZB_BDB_SIGNAL_DEVICE_FIRST_START:
        // Factory-fresh boot, no saved network → form one.
        esp_zb_bdb_start_top_level_commissioning(
            ESP_ZB_BDB_MODE_NETWORK_FORMATION);
        break;

    case ESP_ZB_BDB_SIGNAL_DEVICE_REBOOT:
        // Existing network restored from NVS. Keep permit-join CLOSED —
        // adding new devices later requires an explicit open command
        // (REST endpoint in Phase C). If the reboot signal carries an
        // error, fall back to forming a new network.
        if (status != ESP_OK) {
            esp_zb_bdb_start_top_level_commissioning(
                ESP_ZB_BDB_MODE_NETWORK_FORMATION);
        }
        break;

    case ESP_ZB_ZDO_SIGNAL_DEVICE_ANNCE: {
        // A device just joined / rejoined. Inform the sink so the registry
        // can mint a NodeId from the IEEE address.
        const auto* p = static_cast<const esp_zb_zdo_signal_device_annce_params_t*>(
            esp_zb_app_signal_get_params(signal_struct->p_app_signal));
        if (p != nullptr && s_sink != nullptr) {
            s_sink->onDeviceAnnounced(p->device_short_addr,
                                      ieee_le_to_u64(p->ieee_addr));
        }
        break;
    }

    case ESP_ZB_ZDO_SIGNAL_LEAVE_INDICATION: {
        const auto* p = static_cast<const esp_zb_zdo_signal_leave_indication_params_t*>(
            esp_zb_app_signal_get_params(signal_struct->p_app_signal));
        if (p != nullptr && s_sink != nullptr) {
            s_sink->onDeviceLeft(p->short_addr);
        }
        break;
    }

    default:
        break;
    }
}

// ---------------------------------------------------------------------------
// Zigbee main-loop task.
// Stack: 8 KB (bytes — ESP-IDF FreeRTOS deviation). Priority: just above idle.
// ---------------------------------------------------------------------------
static void zb_task(void* /*arg*/) {
    const esp_err_t err = esp_zb_start(false);
    if (err != ESP_OK) {
        Serial.printf("[zb] esp_zb_start failed: 0x%x — task exits\n", err);
        vTaskDelete(nullptr);
        return;
    }
    esp_zb_stack_main_loop();
    vTaskDelete(nullptr);
}

}  // namespace

// ---------------------------------------------------------------------------
// ZigbeeCoordinatorAdapter implementation
// ---------------------------------------------------------------------------

ZigbeeCoordinatorAdapter::ZigbeeCoordinatorAdapter(
    const std::array<uint8_t, 8>& ext_pan_id) noexcept
    : ext_pan_id_{ext_pan_id} {
    s_ext_pan_id = ext_pan_id;
}

void ZigbeeCoordinatorAdapter::setReportSink(
    gh::domain::IZigbeeReportSink* sink) noexcept {
    s_sink = sink;
}

gh::domain::ErrorCode
ZigbeeCoordinatorAdapter::writeReportPeriod(uint16_t short_addr,
                                             uint32_t period_s) noexcept {
    if (period_s < gh::protocol::kReportPeriodMinS ||
        period_s > gh::protocol::kReportPeriodMaxS) {
        return gh::domain::ErrorCode::InvalidArgument;
    }
    // Hold the SDK lock for the entire write; the attribute pointer we pass
    // to esp_zb_zcl_write_attr_cmd_req must remain valid only until the
    // call returns (the SDK serialises the payload synchronously).
    constexpr TickType_t kLockTimeoutTicks = pdMS_TO_TICKS(500);
    if (!esp_zb_lock_acquire(kLockTimeoutTicks)) {
        return gh::domain::ErrorCode::NetworkDown;
    }
    uint32_t value = period_s;
    esp_zb_zcl_attribute_t attr{};
    attr.id              = gh::protocol::kAttrBasicReportPeriodS;
    attr.data.type       = ESP_ZB_ZCL_ATTR_TYPE_U32;
    attr.data.size       = sizeof(value);
    attr.data.value      = &value;

    esp_zb_zcl_write_attr_cmd_t cmd{};
    cmd.zcl_basic_cmd.dst_addr_u.addr_short = short_addr;
    cmd.zcl_basic_cmd.dst_endpoint          = gh::protocol::kSensorEndpoint;
    cmd.zcl_basic_cmd.src_endpoint          = 1;
    cmd.address_mode      = ESP_ZB_APS_ADDR_MODE_16_ENDP_PRESENT;
    cmd.clusterID         = gh::protocol::kClusterBasic;
    cmd.attr_number       = 1;
    cmd.attr_field        = &attr;

    const esp_err_t err = esp_zb_zcl_write_attr_cmd_req(&cmd);
    esp_zb_lock_release();
    return err == ESP_OK ? gh::domain::ErrorCode::Ok
                         : gh::domain::ErrorCode::NetworkDown;
}

gh::domain::ErrorCode
ZigbeeCoordinatorAdapter::openPermitJoin(uint16_t duration_s) noexcept {
    if (duration_s == 0 || duration_s > 254) {
        return gh::domain::ErrorCode::InvalidArgument;
    }
    constexpr TickType_t kLockTimeoutTicks = pdMS_TO_TICKS(500);
    if (!esp_zb_lock_acquire(kLockTimeoutTicks)) {
        return gh::domain::ErrorCode::NetworkDown;
    }
    const esp_err_t err =
        esp_zb_bdb_open_network(static_cast<uint8_t>(duration_s));
    esp_zb_lock_release();
    if (err == ESP_OK) {
        recordPermitJoinOpen(static_cast<uint8_t>(duration_s));
        return gh::domain::ErrorCode::Ok;
    }
    return gh::domain::ErrorCode::NetworkDown;
}

bool ZigbeeCoordinatorAdapter::isPermitJoinOpen() const noexcept {
    const uint32_t until =
        s_permit_join_until_ms.load(std::memory_order_acquire);
    return until != 0U && millis() < until;
}

uint8_t ZigbeeCoordinatorAdapter::drainPendingPeriodWrites(uint32_t period_s) noexcept {
    uint8_t wrote = 0;
    for (uint8_t i = 0; i < s_known_count; ++i) {
        if (!s_known_nodes[i].needs_period_write) continue;
        if (writeReportPeriod(s_known_nodes[i].short_addr, period_s)
                == gh::domain::ErrorCode::Ok) {
            s_known_nodes[i].needs_period_write = false;
            ++wrote;
        }
    }
    return wrote;
}

// Incoming Report Attributes from sensor-nodes are addressed to coordinator
// EP1. The ZCL stack drops them unless matching clusters exist in CLIENT role
// on that endpoint (empty cluster list = silent loss before zb_action_handler).
static gh::domain::ErrorCode registerSensorClientClusters_(
    esp_zb_cluster_list_t* cluster_list) noexcept
{
    if (esp_zb_cluster_list_add_basic_cluster(
            cluster_list,
            esp_zb_basic_cluster_create(nullptr),
            ESP_ZB_ZCL_CLUSTER_CLIENT_ROLE) != ESP_OK) {
        return gh::domain::ErrorCode::NetworkDown;
    }

    if (esp_zb_cluster_list_add_power_config_cluster(
            cluster_list,
            esp_zb_power_config_cluster_create(nullptr),
            ESP_ZB_ZCL_CLUSTER_CLIENT_ROLE) != ESP_OK) {
        return gh::domain::ErrorCode::NetworkDown;
    }

    esp_zb_temperature_meas_cluster_cfg_t temp_cfg{};
    if (esp_zb_cluster_list_add_temperature_meas_cluster(
            cluster_list,
            esp_zb_temperature_meas_cluster_create(&temp_cfg),
            ESP_ZB_ZCL_CLUSTER_CLIENT_ROLE) != ESP_OK) {
        return gh::domain::ErrorCode::NetworkDown;
    }

    esp_zb_humidity_meas_cluster_cfg_t hum_cfg{};
    if (esp_zb_cluster_list_add_humidity_meas_cluster(
            cluster_list,
            esp_zb_humidity_meas_cluster_create(&hum_cfg),
            ESP_ZB_ZCL_CLUSTER_CLIENT_ROLE) != ESP_OK) {
        return gh::domain::ErrorCode::NetworkDown;
    }

    esp_zb_attribute_list_t* soil_client =
        esp_zb_zcl_attr_list_create(gh::protocol::kClusterSoilMoisture);
    if (soil_client == nullptr ||
        esp_zb_cluster_list_add_custom_cluster(
            cluster_list, soil_client, ESP_ZB_ZCL_CLUSTER_CLIENT_ROLE) != ESP_OK) {
        return gh::domain::ErrorCode::NetworkDown;
    }

    return gh::domain::ErrorCode::Ok;
}

gh::domain::ErrorCode
ZigbeeCoordinatorAdapter::start(uint32_t initial_permit_ms) noexcept {
    // Convert from milliseconds to seconds (SDK uses seconds for permit-join).
    s_permit_join_s = initial_permit_ms / 1000U;

    // 1. Platform config — native 802.15.4 radio, no host connection.
    esp_zb_platform_config_t platform_config{};
    platform_config.radio_config.radio_mode = ZB_RADIO_MODE_NATIVE;
    platform_config.host_config.host_connection_mode =
        ZB_HOST_CONNECTION_MODE_NONE;
    if (esp_zb_platform_config(&platform_config) != ESP_OK) {
        return gh::domain::ErrorCode::NetworkDown;
    }

    // 2. Zigbee stack config — Coordinator role.
    esp_zb_cfg_t cfg{};
    cfg.esp_zb_role         = ESP_ZB_DEVICE_TYPE_COORDINATOR;
    cfg.install_code_policy = false;
    cfg.nwk_cfg.zczr_cfg.max_children = 10U;
    esp_zb_init(&cfg);

    // 2a. Network identity — pin channel and extended PAN ID so the pair
    //     refuses to cross-bind with any neighbouring Zigbee hub.
    esp_zb_set_primary_network_channel_set(gh::protocol::kZigbeeChannelMask);
    esp_zb_set_extended_pan_id(s_ext_pan_id.data());

    // 2b. Security — replace the well-known ZigbeeAlliance09 default with
    //     our custom Trust Center pre-configured link key, require APS
    //     link-key exchange after join (the per-link derived key is then
    //     used for all further traffic — TC key never re-appears on air),
    //     and reject associations from devices with weak signal.
    esp_zb_secur_TC_standard_preconfigure_key_set(
        const_cast<uint8_t*>(gh::protocol::kZigbeeTcLinkKey));
    esp_zb_secur_link_key_exchange_required_set(true);
    esp_zb_secur_network_min_join_lqi_set(gh::protocol::kZigbeeMinJoinLqi);

    // 3. Register coordinator EP1 with client clusters for sensor reports.
    esp_zb_cluster_list_t* cluster_list = esp_zb_zcl_cluster_list_create();
    if (cluster_list == nullptr) {
        return gh::domain::ErrorCode::NetworkDown;
    }
    if (registerSensorClientClusters_(cluster_list) != gh::domain::ErrorCode::Ok) {
        return gh::domain::ErrorCode::NetworkDown;
    }
    esp_zb_ep_list_t*      ep_list      = esp_zb_ep_list_create();

    esp_zb_endpoint_config_t ep_cfg{};
    ep_cfg.endpoint           = 1U;
    ep_cfg.app_profile_id     = ESP_ZB_AF_HA_PROFILE_ID;
    ep_cfg.app_device_id      = ESP_ZB_HA_HOME_GATEWAY_DEVICE_ID;
    ep_cfg.app_device_version = 0U;

    if (esp_zb_ep_list_add_ep(ep_list, cluster_list, ep_cfg) != ESP_OK) {
        return gh::domain::ErrorCode::NetworkDown;
    }
    if (esp_zb_device_register(ep_list) != ESP_OK) {
        return gh::domain::ErrorCode::NetworkDown;
    }

    // 4. Register action handler for incoming ZCL frames.
    esp_zb_core_action_handler_register(zb_action_handler);

    // 5. Launch Zigbee task.
    // ESP-IDF FreeRTOS xTaskCreate takes stack size in BYTES (deviation from
    // vanilla FreeRTOS which uses words). 8 KB matches Espressif's Zigbee
    // examples — 4 KB was found to be marginal under sustained ZCL traffic.
    xTaskCreate(zb_task,
                "zb_task",
                8192U,
                nullptr,
                tskIDLE_PRIORITY + 1U,
                nullptr);

    return gh::domain::ErrorCode::Ok;
}

gh::domain::ErrorCode ZigbeeCoordinatorAdapter::requestLeave(
    uint16_t short_addr, uint64_t ieee) noexcept
{
    esp_zb_zdo_mgmt_leave_req_param_t req{};
    req.dst_nwk_addr = short_addr;
    for (int i = 0; i < 8; ++i) {
        req.device_address[i] = static_cast<uint8_t>(ieee >> (8 * i));
    }
    req.remove_children = 0;
    req.rejoin          = 0;
    constexpr TickType_t kLockTimeoutTicks = pdMS_TO_TICKS(500);
    if (!esp_zb_lock_acquire(kLockTimeoutTicks)) {
        return gh::domain::ErrorCode::Timeout;
    }
    esp_zb_zdo_device_leave_req(&req, nullptr, nullptr);
    esp_zb_lock_release();
    return gh::domain::ErrorCode::Ok;
}

}  // namespace gh::infra

// NOLINTEND(*-macro-usage)
