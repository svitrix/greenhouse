#pragma once
#include "entities/SystemMode.hpp"
#include "ports/IWifiCredsStore.hpp"
#include "ports/IWifiSta.hpp"
#include "ports/IButton.hpp"
#include "ports/IProvisioningFlagStore.hpp"
#include "ports/IWifiFailCounterStore.hpp"
#include "ports/ILogger.hpp"

namespace gh::app {
class WifiProvisioner {
public:
    WifiProvisioner(gh::domain::IWifiCredsStore&        credsStore,
                    gh::domain::IButton&                button,
                    gh::domain::IWifiSta&               wifiSta,
                    gh::domain::IProvisioningFlagStore& provFlag,
                    gh::domain::IWifiFailCounterStore&  failCounter,
                    gh::domain::ILogger&                log,
                    uint32_t connect_timeout_ms = 30'000,
                    uint8_t  retry_count        = 3,
                    uint16_t button_hold_ms     = 3000) noexcept;

    [[nodiscard]] SystemMode bootstrap() noexcept;

private:
    gh::domain::IWifiCredsStore&        credsStore_;
    gh::domain::IButton&                button_;
    gh::domain::IWifiSta&               wifiSta_;
    gh::domain::IProvisioningFlagStore& provFlag_;
    gh::domain::IWifiFailCounterStore&  failCounter_;
    gh::domain::ILogger&                log_;
    uint32_t connect_timeout_ms_;
    uint8_t  retry_count_;
    uint16_t button_hold_ms_;
};
}
