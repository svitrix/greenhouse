#include "SensorRegistry.hpp"
#include "SensorNodeConfig.hpp"
#include "entities/SensorKind.hpp"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#ifdef ARDUINO
#include <Wire.h>
#include <Arduino.h>
#endif
#pragma GCC diagnostic pop
#include <cstdio>

namespace gh::app {

namespace {
void sleepMs(uint32_t ms) noexcept {
#ifdef ARDUINO
    delay(ms);
#else
    (void)ms;
#endif
}

#if defined(GH_SENSOR_DIAG_LOG) || defined(GH_NODE_NO_SLEEP)
const char* statusLabel(gh::domain::SensorStatus st) noexcept {
    switch (st) {
        case gh::domain::SensorStatus::Ok:      return "ok";
        case gh::domain::SensorStatus::Absent:  return "absent";
        case gh::domain::SensorStatus::Faulty:  return "faulty";
        default:                                return "unprobed";
    }
}

void logProbeDiag(gh::domain::ILogger& log, gh::domain::ISensorChannel& ch,
                  gh::domain::SensorStatus st) noexcept {
    char buf[48] = {0};
    snprintf(buf, sizeof(buf), "probe ch%u %s",
             static_cast<unsigned>(ch.id().value), statusLabel(st));
    log.error("DIAG", buf);
}
#endif

#ifdef ARDUINO
void reinitI2cBus() noexcept {
    Wire.end();
    delay(10);
    Wire.begin(gh::sensor::SensorNodeConfig::kI2cSdaPin,
               gh::sensor::SensorNodeConfig::kI2cSclPin,
               gh::sensor::SensorNodeConfig::kI2cFrequencyHz);
}
#endif
}

bool SensorRegistry::add(gh::domain::ISensorChannel& ch) noexcept {
    if (count_ >= kMaxSensorChannels) return false;
    channels_[count_++] = &ch;
    return true;
}

ChannelSpan SensorRegistry::channels() const noexcept {
    return ChannelSpan{channels_.data(), count_};
}

size_t SensorRegistry::probeAll(gh::domain::IPowerRail& rail,
                                gh::domain::ILogger& log) noexcept {
    rail.on();
    sleepMs(gh::sensor::SensorNodeConfig::kSensorRailSettleMs);
#ifdef ARDUINO
    reinitI2cBus();
    sleepMs(50);
#endif
    // No max-warmup wait here: probe() is meant to be tolerant; each adapter
    // handles its own warmup internally if needed (Chirp's probe reads
    // version after init()).
    size_t ok_count = 0;
    for (size_t i = 0; i < count_; ++i) {
        const auto st = channels_[i]->probe();
#if defined(GH_SENSOR_DIAG_LOG) || defined(GH_NODE_NO_SLEEP)
        logProbeDiag(log, *channels_[i], st);
#endif
        if (st == gh::domain::SensorStatus::Ok) {
            ++ok_count;
        } else if (st == gh::domain::SensorStatus::Absent) {
            log.warn("registry", "sensor absent");
        } else if (st == gh::domain::SensorStatus::Faulty) {
            log.warn("registry", "sensor faulty");
        }
    }
    rail.off();
    return ok_count;
}

uint32_t SensorRegistry::maxWarmupMs() const noexcept {
    uint32_t m = 0;
    for (size_t i = 0; i < count_; ++i) {
        if (channels_[i]->status() == gh::domain::SensorStatus::Ok) {
            const auto w = channels_[i]->warmupMs();
            if (w > m) m = w;
        }
    }
    return m;
}

uint32_t SensorRegistry::presentMask() const noexcept {
    uint32_t mask = 0;
    for (size_t i = 0; i < count_; ++i) {
        if (channels_[i]->status() == gh::domain::SensorStatus::Ok) {
            const auto bit = channels_[i]->id().value;
            if (bit < 32) mask |= (1u << bit);
        }
    }
    return mask;
}

}
