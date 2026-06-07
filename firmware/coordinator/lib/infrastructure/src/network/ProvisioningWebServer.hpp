#pragma once
#include <ESPAsyncWebServer.h>
#include "ports/IAdminCredsStore.hpp"
#include "ports/IAnalyticsConfigStore.hpp"
#include "ports/ILastConnectErrorStore.hpp"
#include "ports/ILogger.hpp"
#include "ports/IMqttCredsStore.hpp"
#include "ports/IPairingClient.hpp"
#include "ports/IProvisioningFlagStore.hpp"
#include "ports/ISoilCalibrationStore.hpp"
#include "ports/ISystemInfo.hpp"
#include "ports/IWifiCredsStore.hpp"

namespace gh::infra {
class ProvisioningWebServer {
public:
    ProvisioningWebServer(gh::domain::IWifiCredsStore&         wifi,
                          gh::domain::IMqttCredsStore&         mqtt,
                          gh::domain::ISoilCalibrationStore&   soil,
                          gh::domain::IProvisioningFlagStore&  prov_flag,
                          gh::domain::ILastConnectErrorStore&  last_error,
                          gh::domain::IAdminCredsStore&        admin_creds,
                          gh::domain::IAnalyticsConfigStore&   analytics,
                          gh::domain::IPairingClient&          pairing,
                          gh::domain::ISystemInfo&             sysinfo,
                          gh::domain::ILogger&                 log) noexcept;

    void start() noexcept;

private:
    void registerCaptiveProbes_(AsyncWebServer& server) noexcept;

    AsyncWebServer server_{80};
    gh::domain::IWifiCredsStore&         wifi_;
    gh::domain::IMqttCredsStore&         mqtt_;
    gh::domain::ISoilCalibrationStore&   soil_;
    gh::domain::IProvisioningFlagStore&  prov_flag_;
    gh::domain::ILastConnectErrorStore&  last_error_;
    gh::domain::IAdminCredsStore&        admin_creds_;
    gh::domain::IAnalyticsConfigStore&   analytics_;
    gh::domain::IPairingClient&          pairing_;
    gh::domain::ISystemInfo&             sysinfo_;
    gh::domain::ILogger&                 log_;
    uint32_t last_scan_trigger_ms_ = 0U;  // 0 = scan never triggered
};
}
