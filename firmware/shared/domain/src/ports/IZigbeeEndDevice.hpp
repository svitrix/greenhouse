#pragma once
#include <cstddef>
#include <cstdint>
#include "errors/ErrorCode.hpp"

namespace gh::domain {

// ZCL attribute type tag. Values match ESP_ZB_ZCL_ATTR_TYPE_* in
// esp_zigbee_zcl_common.h (verified against the esp32c6 SDK header):
//   ESP_ZB_ZCL_ATTR_TYPE_U8  = 0x20
//   ESP_ZB_ZCL_ATTR_TYPE_U16 = 0x21
//   ESP_ZB_ZCL_ATTR_TYPE_U32 = 0x23
//   ESP_ZB_ZCL_ATTR_TYPE_S16 = 0x29
enum class ZclType : uint8_t {
    Uint8  = 0x20,   // matches ESP_ZB_ZCL_ATTR_TYPE_U8
    Uint16 = 0x21,   // matches ESP_ZB_ZCL_ATTR_TYPE_U16
    Uint32 = 0x23,   // matches ESP_ZB_ZCL_ATTR_TYPE_U32
    Int16  = 0x29,   // matches ESP_ZB_ZCL_ATTR_TYPE_S16
};

// Sensor-node-side Zigbee adapter. Sleepy end device.
struct IZigbeeEndDevice {
    virtual ~IZigbeeEndDevice() = default;

    // start() — blocks until either attached to a network or
    // steering_timeout_ms has elapsed. Returns:
    //   Ok                        — joined / re-attached
    //   ZigbeeJoinTimeout         — steering window expired, no parent found
    //   ZigbeeStackInitFailed     — esp_zb stack/cluster/register failure (never on air)
    //   ZigbeeTrustCenterMismatch — joined a TC that fails key verification
    [[nodiscard]] virtual ErrorCode start(uint32_t steering_timeout_ms) noexcept = 0;

    // Generic single-attribute report. Returns Ok or MqttDisconnected
    // (reused for "tx ack timeout"). Used by ZigbeeReportMapper.
    // `size` argument is informational — the SDK uses the attribute's registered size.
    [[nodiscard]] virtual ErrorCode reportAttribute(
        uint8_t  endpoint,
        uint16_t cluster_id,
        uint16_t attribute_id,
        ZclType  type,
        const void* data,
        size_t   size,
        uint32_t tx_timeout_ms) noexcept = 0;

    // Reads the value of custom attribute 0xFF00 (report_period_s) from the local
    // attribute store. Returns a sane default if not configured by the coordinator.
    [[nodiscard]] virtual uint32_t reportPeriodSeconds() const noexcept = 0;
};

}
