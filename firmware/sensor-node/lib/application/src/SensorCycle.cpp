#include "SensorCycle.hpp"
#include "SensorNodeConfig.hpp"
#include "ZigbeeReportMapper.hpp"
#include "entities/SensorKind.hpp"
#include <cstdio>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#ifdef ARDUINO
#include <Arduino.h>
#include <Wire.h>
#endif
#pragma GCC diagnostic pop

namespace gh::sensor {

namespace {
constexpr size_t kMaxReadings = gh::app::kMaxSensorChannels;

void sleepMs(uint32_t ms) noexcept {
#ifdef ARDUINO
    delay(ms);
#else
    (void)ms;
#endif
}

#if defined(GH_SENSOR_DIAG_LOG) || defined(GH_NODE_NO_SLEEP)
void logReading_(gh::domain::ILogger& log,
                 const gh::domain::SensorReading& r) noexcept {
    char buf[96] = {0};
    switch (r.kind) {
    case gh::domain::SensorKind::Air: {
        const int16_t  t_x10 = r.values.air.temperature_c_x10;
        const uint16_t h_x10 = r.values.air.humidity_pct_x10;
        const int t_frac = (t_x10 < 0 ? -(t_x10 % 10) : t_x10 % 10);
        snprintf(buf, sizeof(buf), "air T=%d.%dC H=%u.%u%%",
                 static_cast<int>(t_x10 / 10), t_frac,
                 static_cast<unsigned>(h_x10 / 10),
                 static_cast<unsigned>(h_x10 % 10));
        break;
    }
    case gh::domain::SensorKind::Soil: {
        const int16_t t_x10 = r.values.soil.temperature_c_x10;
        const int t_frac = (t_x10 < 0 ? -(t_x10 % 10) : t_x10 % 10);
        snprintf(buf, sizeof(buf), "soil raw=%u T=%d.%dC",
                 static_cast<unsigned>(r.values.soil.raw_capacitance),
                 static_cast<int>(t_x10 / 10), t_frac);
        break;
    }
    case gh::domain::SensorKind::Battery:
        snprintf(buf, sizeof(buf), "bat %umV %u%%",
                 static_cast<unsigned>(r.values.battery.voltage_mv),
                 static_cast<unsigned>(r.values.battery.state_of_charge_pct));
        break;
    default:
        snprintf(buf, sizeof(buf), "ch%u kind=%u",
                 static_cast<unsigned>(r.id.value),
                 static_cast<unsigned>(static_cast<uint8_t>(r.kind)));
        break;
    }
    log.info("cycle", buf);
}
#endif
}

SensorCycle::SensorCycle(gh::app::SensorRegistry&       registry,
                          gh::domain::IPowerRail&        rail,
                          gh::infra::ZigbeeReportMapper& mapper,
                          gh::domain::IZigbeeEndDevice&  zb,
                          gh::domain::IDeepSleep&        sleep,
                          gh::domain::IClock&            clock,
                          gh::domain::ILogger&           log,
                          uint32_t                       tx_timeout_ms) noexcept
    : registry_(registry), rail_(rail), mapper_(mapper), zb_(zb),
      sleep_(sleep), clock_(clock), log_(log),
      tx_timeout_ms_(tx_timeout_ms) {}

uint32_t SensorCycle::runOnce() noexcept {
    const uint32_t boot_t_ms = clock_.nowMs();

    rail_.on();
    sleepMs(registry_.maxWarmupMs());
#ifdef ARDUINO
    Wire.end();
    delay(10);
    Wire.begin(SensorNodeConfig::kI2cSdaPin, SensorNodeConfig::kI2cSclPin,
               SensorNodeConfig::kI2cFrequencyHz);
    delay(50);
#endif

    gh::domain::SensorReading readings[kMaxReadings];
    size_t reading_count = 0;
    const auto chans = registry_.channels();
    for (size_t i = 0; i < chans.size; ++i) {
        if (chans.data[i]->status() != gh::domain::SensorStatus::Ok) continue;
        auto r = chans.data[i]->read();
        if (r.ok()) {
            r.value.read_at_ms = boot_t_ms;
            readings[reading_count++] = r.value;
        } else {
            log_.warn("cycle", "channel read failed");
        }
    }

#if defined(GH_SENSOR_DIAG_LOG) || defined(GH_NODE_NO_SLEEP)
    {
        char summary[48] = {0};
        const uint32_t mask = registry_.presentMask();
        snprintf(summary, sizeof(summary), "publish mask=0x%02X count=%u",
                 static_cast<unsigned>(mask & 0xFFu),
                 static_cast<unsigned>(reading_count));
        log_.info("cycle", summary);
        for (size_t i = 0; i < reading_count; ++i) {
            logReading_(log_, readings[i]);
        }
        log_.error("DIAG", summary);
        for (size_t i = 0; i < reading_count; ++i) {
            char tag[48] = {0};
            switch (readings[i].kind) {
            case gh::domain::SensorKind::Air: {
                const int16_t t = readings[i].values.air.temperature_c_x10;
                const uint16_t h = readings[i].values.air.humidity_pct_x10;
                snprintf(tag, sizeof(tag), "read air T=%d.%d H=%u.%u",
                         static_cast<int>(t / 10),
                         static_cast<int>(t < 0 ? -(t % 10) : t % 10),
                         static_cast<unsigned>(h / 10),
                         static_cast<unsigned>(h % 10));
                break;
            }
            case gh::domain::SensorKind::Soil:
                snprintf(tag, sizeof(tag), "read soil raw=%u T=%d.%d",
                         static_cast<unsigned>(readings[i].values.soil.raw_capacitance),
                         static_cast<int>(readings[i].values.soil.temperature_c_x10 / 10),
                         static_cast<int>(readings[i].values.soil.temperature_c_x10 % 10));
                break;
            case gh::domain::SensorKind::Battery:
                snprintf(tag, sizeof(tag), "read bat %umV %u%%",
                         static_cast<unsigned>(readings[i].values.battery.voltage_mv),
                         static_cast<unsigned>(readings[i].values.battery.state_of_charge_pct));
                break;
            default:
                snprintf(tag, sizeof(tag), "read ch%u", static_cast<unsigned>(readings[i].id.value));
                break;
            }
            log_.error("DIAG", tag);
        }
    }
#endif

    mapper_.publish(readings, reading_count,
                    registry_.presentMask(),
                    tx_timeout_ms_);

    rail_.off();
    return zb_.reportPeriodSeconds() * 1000u;
}

void SensorCycle::run() noexcept {
    const uint32_t sleep_ms = runOnce();
    log_.info("cycle", "sleeping");
    sleep_.sleepFor(sleep_ms);
    __builtin_unreachable();
}

}
