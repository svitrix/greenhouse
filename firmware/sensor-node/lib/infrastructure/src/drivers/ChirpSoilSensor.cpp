// Arduino-ESP32 headers pulled in via Wire.h (WString.h, Stream.h) have
// benign -Wconversion issues we cannot fix upstream. Suppress around the
// SDK includes (including those transitively pulled in by ChirpSoilSensor.hpp).
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#include "ChirpSoilSensor.hpp"
#include <Arduino.h>
#pragma GCC diagnostic pop
#include <cstring>

namespace {
constexpr uint8_t  kRegGetCapacitance  = 0x00;
constexpr uint8_t  kRegGetTemperature  = 0x05;
constexpr uint8_t  kRegReset           = 0x06;
constexpr uint8_t  kRegGetVersion      = 0x07;
constexpr uint8_t  kRegBusy            = 0x09;
constexpr uint32_t kPostResetWarmupMs  = 1000;  // Chirp README: ≥1 s after soft-reset
constexpr uint32_t kMeasurementMs      = 15;
constexpr uint32_t kBusyTimeoutMs      = 50;
constexpr uint32_t kBusyPollIntervalMs = 2;
constexpr uint8_t  kMinFirmwareVersion = 0x22;
}

namespace gh::infra {

ChirpSoilSensor::ChirpSoilSensor(TwoWire& bus, uint8_t address,
                                 gh::domain::SensorChannelId channel_id) noexcept
    : bus_(bus), address_(address), channel_id_(channel_id) {}

gh::domain::ErrorCode
ChirpSoilSensor::endTxToError_(uint8_t code) noexcept {
    switch (code) {
        case 0:  return gh::domain::ErrorCode::Ok;
        case 2:  return gh::domain::ErrorCode::I2cNack;  // NACK on address
        case 3:  return gh::domain::ErrorCode::I2cNack;  // NACK on data
        default: return gh::domain::ErrorCode::I2cTimeout;
    }
}

gh::domain::ErrorCode
ChirpSoilSensor::writeCommand_(uint8_t cmd) noexcept {
    bus_.beginTransmission(address_);
    bus_.write(cmd);
    return endTxToError_(bus_.endTransmission());
}

gh::domain::Result<uint8_t>
ChirpSoilSensor::readByte_(uint8_t cmd) noexcept {
    using R = gh::domain::Result<uint8_t>;
    if (auto err = writeCommand_(cmd); err != gh::domain::ErrorCode::Ok) {
        return R::failure(err);
    }
    if (bus_.requestFrom(address_, static_cast<uint8_t>(1)) != 1) {
        return R::failure(gh::domain::ErrorCode::I2cTimeout);
    }
    return R::success(static_cast<uint8_t>(bus_.read()));
}

gh::domain::Result<uint16_t>
ChirpSoilSensor::writeRegAndRead16_(uint8_t cmd) noexcept {
    using R = gh::domain::Result<uint16_t>;
    if (auto err = writeCommand_(cmd); err != gh::domain::ErrorCode::Ok) {
        return R::failure(err);
    }
    if (bus_.requestFrom(address_, static_cast<uint8_t>(2)) != 2) {
        return R::failure(gh::domain::ErrorCode::I2cTimeout);
    }
    const uint16_t hi = static_cast<uint16_t>(bus_.read());
    const uint16_t lo = static_cast<uint16_t>(bus_.read());
    return R::success(static_cast<uint16_t>((hi << 8) | lo));
}

gh::domain::ErrorCode ChirpSoilSensor::waitNotBusy_() noexcept {
    const uint32_t deadline = millis() + kBusyTimeoutMs;
    while (millis() < deadline) {
        auto r = readByte_(kRegBusy);
        if (!r.ok()) return r.err;
        if (r.value == 0) return gh::domain::ErrorCode::Ok;
        delay(kBusyPollIntervalMs);
    }
    // Stuck BUSY — recover the bus and surface to caller.
    bus_.end();
    bus_.begin();
    return gh::domain::ErrorCode::I2cTimeout;
}

gh::domain::ErrorCode ChirpSoilSensor::init() noexcept {
    if (auto err = writeCommand_(kRegReset);
        err != gh::domain::ErrorCode::Ok) {
        return err;
    }
    delay(kPostResetWarmupMs);

    auto ver = readByte_(kRegGetVersion);
    if (!ver.ok()) return ver.err;
    if (ver.value < kMinFirmwareVersion) {
        return gh::domain::ErrorCode::SensorVersionMismatch;
    }

    initialised_ = true;
    return gh::domain::ErrorCode::Ok;
}

gh::domain::Result<gh::domain::SoilSample>
ChirpSoilSensor::readSoil() noexcept {
    using R = gh::domain::Result<gh::domain::SoilSample>;
    if (!initialised_) {
        return R::failure(gh::domain::ErrorCode::SensorNotReady);
    }

    // Wait for any prior measurement to complete before triggering a new one.
    // If BUSY stays asserted, recover the bus and surface as I2cTimeout —
    // the cycle's all-or-nothing policy will skip this report.
    if (auto err = waitNotBusy_(); err != gh::domain::ErrorCode::Ok) {
        return R::failure(err);
    }

    // Capacitance: first read returns stale value and triggers new measurement.
    // Discard it, wait for measurement to complete, then read fresh value.
    if (auto trig = writeRegAndRead16_(kRegGetCapacitance); !trig.ok()) {
        return R::failure(trig.err);
    }
    delay(kMeasurementMs);
    auto cap = writeRegAndRead16_(kRegGetCapacitance);
    if (!cap.ok()) return R::failure(cap.err);

    // Temperature: same trigger-then-read pattern. Interpret as int16 (signed!).
    if (auto trig = writeRegAndRead16_(kRegGetTemperature); !trig.ok()) {
        return R::failure(trig.err);
    }
    delay(kMeasurementMs);
    auto temp = writeRegAndRead16_(kRegGetTemperature);
    if (!temp.ok()) return R::failure(temp.err);

    // moisture_pct stays 0 (default-init by SoilSample{}); coordinator-side
    // SoilNormalizer fills it from raw_capacitance + calibration.
    gh::domain::SoilSample sample{};
    sample.timestamp_ms      = millis();
    sample.raw_capacitance   = cap.value;
    int16_t signed_temp = 0;
    std::memcpy(&signed_temp, &temp.value, sizeof(signed_temp));
    sample.temperature_c_x10 = signed_temp;
    return R::success(sample);
}

gh::domain::Result<gh::domain::SensorReading>
ChirpSoilSensor::read() noexcept {
    using R = gh::domain::Result<gh::domain::SensorReading>;
    auto soil = readSoil();
    if (!soil.ok()) {
        status_ = gh::domain::SensorStatus::Faulty;
        return R::failure(soil.err);
    }
    status_ = gh::domain::SensorStatus::Ok;
    gh::domain::SensorReading r{};
    r.id          = channel_id_;
    r.kind        = gh::domain::SensorKind::Soil;
    r.read_at_ms  = soil.value.timestamp_ms;
    r.values.soil = soil.value;
    return R::success(r);
}

gh::domain::SensorStatus ChirpSoilSensor::probe() noexcept {
    // Tolerant probe: init then read GET_VERSION (0x07). Chirp returns 0x22/0x23.
    // NACK/timeout -> Absent; other errors -> Faulty.
    if (init() != gh::domain::ErrorCode::Ok) {
        status_ = gh::domain::SensorStatus::Absent;
        return status_;
    }
    auto v = readByte_(kRegGetVersion);
    if (!v.ok()) {
        status_ = (v.err == gh::domain::ErrorCode::I2cNack ||
                   v.err == gh::domain::ErrorCode::I2cTimeout)
                  ? gh::domain::SensorStatus::Absent
                  : gh::domain::SensorStatus::Faulty;
        return status_;
    }
    status_ = gh::domain::SensorStatus::Ok;
    return status_;
}

}
