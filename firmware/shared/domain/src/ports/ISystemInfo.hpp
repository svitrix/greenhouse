#pragma once
#include <cstdint>

namespace gh::domain {

struct SystemInfo {
    char     device_id[20];        // "greenhouse_a1b2c3" (18 chars + NUL, padded to 20)
    char     firmware_version[16]; // "0.4.0"
    char     ip[16];               // "192.168.1.42"
    uint32_t uptime_s;
    int16_t  wifi_rssi_dbm;
};

struct ISystemInfo {
    virtual ~ISystemInfo() = default;
    virtual void snapshot(SystemInfo& out) noexcept = 0;
};

}
