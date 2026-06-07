#include "WifiProvisioner.hpp"
#include "CoordinatorConfig.hpp"

namespace gh::app {

using gh::domain::ErrorCode;

WifiProvisioner::WifiProvisioner(gh::domain::IWifiCredsStore&        credsStore,
                                 gh::domain::IButton&                button,
                                 gh::domain::IWifiSta&               wifiSta,
                                 gh::domain::IProvisioningFlagStore& provFlag,
                                 gh::domain::IWifiFailCounterStore&  failCounter,
                                 gh::domain::ILogger&                log,
                                 uint32_t connect_timeout_ms,
                                 uint8_t  retry_count,
                                 uint16_t button_hold_ms) noexcept
    : credsStore_(credsStore), button_(button), wifiSta_(wifiSta),
      provFlag_(provFlag), failCounter_(failCounter), log_(log),
      connect_timeout_ms_(connect_timeout_ms), retry_count_(retry_count),
      button_hold_ms_(button_hold_ms) {}

SystemMode WifiProvisioner::bootstrap() noexcept {
    if (provFlag_.isForced()) {
        log_.info("provisioner", "force_provisioning flag set");
        (void) failCounter_.reset();
        return SystemMode::Provisioning;
    }

    if (button_.holdConfirmed(button_hold_ms_)) {
        log_.info("provisioner", "BOOT button held");
        (void) failCounter_.reset();
        return SystemMode::Provisioning;
    }

    if (failCounter_.load() >= gh::coord::CoordinatorConfig::kMaxConsecutiveFailedBoots) {
        log_.warn("provisioner", "fail-counter threshold, forcing provisioning");
        (void) failCounter_.reset();
        return SystemMode::Provisioning;
    }

    auto loaded = credsStore_.load();
    if (!loaded.ok()) {
        log_.info("provisioner", "no creds in NVS, entering provisioning");
        (void) failCounter_.reset();
        return SystemMode::Provisioning;
    }
    if (!loaded.value.valid()) {
        log_.warn("provisioner", "invalid creds in NVS, entering provisioning");
        (void) failCounter_.reset();
        return SystemMode::Provisioning;
    }

    for (uint8_t i = 0; i < retry_count_; ++i) {
        const auto err = wifiSta_.connect(loaded.value, connect_timeout_ms_);
        if (err == ErrorCode::Ok) {
            log_.info("provisioner", "STA connected");
            // failCounter.reset() happens in WifiStaAdapter on success (Task 3.7)
            return SystemMode::Operational;
        }
        log_.warn("provisioner", "STA connect attempt failed");
    }
    log_.warn("provisioner", "STA failed after retries, entering provisioning");
    (void) failCounter_.increment();
    return SystemMode::Provisioning;
}

}
