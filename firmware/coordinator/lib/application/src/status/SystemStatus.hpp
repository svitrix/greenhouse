#pragma once
#include <cstdint>

namespace gh::app {

// Board status surfaced on the on-board RGB LED. Ordered by severity so the
// arbiter can pick the highest-priority active condition (see
// StatusLedService::arbitrate).
enum class SystemStatus : uint8_t {
    Boot,            // bring-up, before mode is decided
    Provisioning,    // captive-portal AP up, waiting for Wi-Fi setup
    WifiConnecting,  // STA association in progress
    Online,          // Wi-Fi (+ expected MQTT) up, nominal
    Degraded,        // Wi-Fi up but MQTT expected and not connected
    Watering,        // pump running
    Fault,           // pump safety lock (dry-run / max-runtime)
    ZigbeePairing,   // Zigbee network up, permit-join window open
};

}
