#pragma once
#include <cstdint>

namespace gh::domain {
enum class ErrorCode : uint8_t {
    Ok = 0,
    I2cTimeout, I2cCrc, I2cNack,
    SensorNotReady, SensorOutOfRange, SensorVersionMismatch, SensorTooFast,
    PumpSafetyTimeout, PumpFloatDry, PumpLocked,
    NetworkDown, MqttDisconnected,
    WifiNotProvisioned, WifiConnectFailed, WifiAPModeFailed, HttpFormInvalid,
    ValidationFailed,
    ConfigNotFound, ConfigStoreFailed,
    ZigbeeTrustCenterMismatch,
    InvalidArgument,
    HttpTransportFailure,
    HttpTimeout,
    TlsHandshakeFailed,
    QueueFull,
    QueueIoFailure,
    FsMountFailed,
    NvsAccessFailed,
    NotFound,
    PairingWindowExpired,
    PairingDeviceConflict,
    PairingProfileUnknown,
    Unknown,
    BoundedStorageExceeded,   // node registry / history store full
    AliasTooLong,             // node alias > 23 chars
    NodeUnknown,              // operation references unregistered IEEE
    Timeout,                  // generic timeout (e.g. Zigbee Mgmt_Leave_req ack)
    ZigbeeStackInitFailed,    // esp_zb platform/cluster-build/device_register failed (never on air)
    ZigbeeJoinTimeout,        // steering window expired, no parent found
};
}
