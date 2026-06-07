#include "DeepSleepClock.hpp"
#include <esp_sleep.h>

namespace gh::infra {

void DeepSleepClock::sleepFor(uint32_t wake_up_ms) noexcept {
    const uint64_t wake_us = static_cast<uint64_t>(wake_up_ms) * 1000ULL;
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
    esp_sleep_enable_timer_wakeup(wake_us);
    esp_deep_sleep_start();
    // unreachable
    while (true) {}
}

}
