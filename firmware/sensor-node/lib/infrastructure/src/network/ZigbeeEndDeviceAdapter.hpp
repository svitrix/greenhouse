#pragma once
#include <cstdint>
#include "ports/IZigbeeEndDevice.hpp"
#include "ports/ILogger.hpp"
#include "ports/IRgbLed.hpp"

namespace gh::infra {

// Zigbee sleepy End Device adapter for the sensor-node firmware.
//
// Implements IZigbeeEndDevice using espressif esp-zigbee-lib (already bundled
// in the pioarduino framework-arduinoespressif32-libs package for esp32c6).
//
// Registers endpoint 1 with 5 server clusters:
//   - Basic (0x0000): manufacturer name, model ID, custom 0xFF00 report_period_s attr
//   - Power Configuration (0x0001): BatteryPercentageRemaining (0x0021)
//   - Temperature Measurement (0x0402): MeasuredValue (0x0000)
//   - Relative Humidity (0x0405): MeasuredValue (0x0000)
//   - Soil Moisture (0x0408): MeasuredValue (0x0000, custom cluster)
//
// start() blocks the calling task until the device joins the network or
// steering_timeout_ms elapses.
//
// reportPeriodSeconds() reads attribute 0xFF00 from the local attribute store
// and returns a value clamped to [kReportPeriodMinS, kReportPeriodMaxS].

class ZigbeeEndDeviceAdapter final : public gh::domain::IZigbeeEndDevice {
public:
    explicit ZigbeeEndDeviceAdapter(gh::domain::ILogger& log,
                                    gh::domain::IRgbLed& pairing_led) noexcept;

    // Blocks until joined or timeout. Returns:
    //   Ok                        — joined / re-attached
    //   ZigbeeJoinTimeout         — steering window expired, no parent found
    //   ZigbeeStackInitFailed     — esp_zb stack/cluster/register failure (never on air)
    //   ZigbeeTrustCenterMismatch — joined a TC that fails key verification
    [[nodiscard]] gh::domain::ErrorCode
    start(uint32_t steering_timeout_ms) noexcept override;

    // Generic single-attribute report. Writes the value to the local ZCL
    // attribute store, issues Report Attributes for that one attribute, and
    // waits for the APS confirm (up to tx_timeout_ms).
    // Returns Ok or MqttDisconnected (reused for "tx ack timeout").
    // `size` argument is informational — the SDK uses the attribute's registered size.
    [[nodiscard]] gh::domain::ErrorCode
    reportAttribute(uint8_t  endpoint,
                    uint16_t cluster_id,
                    uint16_t attribute_id,
                    gh::domain::ZclType type,
                    const void* data,
                    size_t   size,
                    uint32_t tx_timeout_ms) noexcept override;

    // Reads attribute 0xFF00 from the local Basic cluster store.
    // Returns kReportPeriodDefaultS if not yet written by coordinator.
    [[nodiscard]] uint32_t reportPeriodSeconds() const noexcept override;

    // Erase the saved Trust Center pairing (NVS namespace "zigbee_pair").
    // Call from the composition root on TC mismatch so the next boot does
    // fresh steering instead of immediately mismatching the same saved key.
    static void clearPairingNvs() noexcept;

    // Returns the last ZCL init error captured by ZB_RETURN_IF_FAIL.
    // "none" if start() has not failed yet. Safe to call from any context.
    static const char* initErrContext() noexcept;

private:
    // All ZCL cluster/endpoint building and esp_zb_device_register().
    // Called from start(); zb_task is always created after this returns,
    // whether success or failure, so esp_zb_init() is never left unstarted.
    [[nodiscard]] gh::domain::ErrorCode buildAndRegisterEndpoints_() noexcept;

    void tickPairingLed_(uint32_t now_ms) noexcept;
    void pairingLedOff_() noexcept;

    gh::domain::ILogger&  log_;
    gh::domain::IRgbLed&  pairing_led_;
    bool                  pairing_led_on_ = false;
};

}  // namespace gh::infra
