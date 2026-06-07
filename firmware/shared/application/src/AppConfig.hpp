#pragma once
#include <cstdint>
#include "entities/SoilCalibration.hpp"

namespace gh::app {
// Hardware constants below are mirrored in the canonical doc table —
// docs/hardware/reference/canonical-values.md. This header is the runtime
// truth; if the two ever diverge, update the doc to match (see that file's
// "maintenance rule").
struct AppConfig {
    // I²C — docs/hardware/reference/canonical-values.md#i2c
    static constexpr uint8_t  kI2cSdaPin           = 6;
    static constexpr uint8_t  kI2cSclPin           = 7;
    static constexpr uint32_t kI2cFrequencyHz      = 100'000;
    static constexpr uint8_t  kChirpAddress        = 0x20;
    static constexpr uint8_t  kAm2315cAddress      = 0x38;

    // GPIO — docs/hardware/reference/canonical-values.md#coordinator-gpio
    // Pump relay (channel 1) on J3.9 — digital-only, non-strapping GPIO.
    // GPIO10/11 are NOT exposed on DevKitM-1 (internal SPI-flash pins).
    static constexpr uint8_t  kPumpRelayGpio       = 18;
    static constexpr bool     kPumpRelayActiveHigh = true;

    // Timings
    static constexpr uint32_t kSensorPeriodMs      = 10'000;
    static constexpr uint32_t kIrrigationTickMs    = 1'000;
    static constexpr uint32_t kPumpMaxRuntimeMs    = 20'000;
    static constexpr uint32_t kMqttReconnectMs     = 5'000;
    static constexpr uint32_t kWifiBootTimeoutMs   = 30'000;

    // Soil calibration default — GENERIC fallback only, used when NVS soil_calib
    // is empty. Intentionally NOT a specific sensor's numbers. The measured
    // per-unit values (e.g. 249/489) are stored in NVS and override this.
    // Canonical: docs/hardware/reference/canonical-values.md#calibration
    static constexpr gh::domain::SoilCalibration kDefaultSoilCalibration{
        .raw_dry = 300,   // Chirp in air ~290..310 (per README)
        .raw_wet = 700    // Chirp submerged in water lower bound
    };

    // Zigbee
    // Desired report period for sensor-nodes. Coordinator writes this via
    // Write Attribute 0xFF00 to each newly-seen sensor short-addr.
    static constexpr uint32_t kReportPeriodS = 60;

    // Provisioning
    // GPIO9 is the onboard BOOT button on ESP32-C6-DevKitM-1 AND a strapping pin.
    // The dev-board already wires it through a pull-up + button to GND, so INPUT_PULLUP
    // works out of the box. DO NOT connect external loads here: the pin MUST be high-Z
    // during reset, otherwise the chip enters Firmware Download mode and won't boot.
    // See CLAUDE.md §0.1 (strapping pins).
    static constexpr uint8_t  kBootButtonGpio           = 9;
    static constexpr uint16_t kProvisioningButtonHoldMs = 3000;
    static constexpr uint32_t kStaConnectTimeoutMs      = 30'000;
    static constexpr uint8_t  kStaRetryCount            = 3;
    static constexpr const char* kApSsidPrefix          = "Greenhouse-Setup";
    static constexpr uint16_t kProvisioningWebPort      = 80;
};
}
