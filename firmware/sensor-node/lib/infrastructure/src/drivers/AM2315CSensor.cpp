// Arduino-ESP32 headers pulled in via Wire.h (WString.h, Stream.h) have
// benign -Wconversion issues we cannot fix upstream. Suppress around the
// SDK includes (including those transitively pulled in by AM2315CSensor.hpp).
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#include "AM2315CSensor.hpp"
#include <Arduino.h>
#pragma GCC diagnostic pop

namespace {
constexpr uint32_t kBusyPollIntervalMs   = 10;
constexpr uint32_t kBusyTimeoutMs        = 100;
}

namespace gh::infra {

AM2315CSensor::AM2315CSensor(TwoWire& bus, uint8_t address) noexcept
    : bus_(bus), address_(address) {}

gh::domain::ErrorCode AM2315CSensor::init() noexcept {
    if (auto err = resetQuirkIfNeeded_();
        err != gh::domain::ErrorCode::Ok) {
        return err;
    }
    initialised_ = true;
    return gh::domain::ErrorCode::Ok;
}

gh::domain::Result<gh::domain::AirSample>
AM2315CSensor::readAir() noexcept {
    using R = gh::domain::Result<gh::domain::AirSample>;
    if (!initialised_) return R::failure(gh::domain::ErrorCode::SensorNotReady);

    if (auto err = triggerMeasurement_();
        err != gh::domain::ErrorCode::Ok) {
        return R::failure(err);
    }

    if (auto err = waitNotBusy_(kBusyTimeoutMs);
        err != gh::domain::ErrorCode::Ok) {
        return R::failure(err);
    }

    uint8_t bits[7] = {0};
    if (auto err = readBytes_(bits, 7);
        err != gh::domain::ErrorCode::Ok) {
        return R::failure(err);
    }

    if (crc8_(bits, 6) != bits[6]) {
        return R::failure(gh::domain::ErrorCode::I2cCrc);
    }

    const uint32_t raw_h = (static_cast<uint32_t>(bits[1]) << 12)
                         | (static_cast<uint32_t>(bits[2]) << 4)
                         | (static_cast<uint32_t>(bits[3]) >> 4);
    const uint32_t raw_t = (static_cast<uint32_t>(bits[3] & 0x0F) << 16)
                         | (static_cast<uint32_t>(bits[4]) << 8)
                         |  static_cast<uint32_t>(bits[5]);

    const uint16_t humidity_pct_x10 =
        static_cast<uint16_t>((raw_h * 1000UL) >> 20);
    const int16_t temperature_c_x10 = static_cast<int16_t>(
        static_cast<int32_t>((raw_t * 2000UL) >> 20) - 500);

    gh::domain::AirSample sample{};
    sample.timestamp_ms      = millis();
    sample.temperature_c_x10 = temperature_c_x10;
    sample.humidity_pct_x10  = humidity_pct_x10;
    return R::success(sample);
}

gh::domain::Result<gh::domain::SensorReading>
AM2315CSensor::read() noexcept {
    using R = gh::domain::Result<gh::domain::SensorReading>;
    auto air = readAir();
    if (!air.ok()) {
        status_ = gh::domain::SensorStatus::Faulty;
        return R::failure(air.err);
    }
    status_ = gh::domain::SensorStatus::Ok;
    gh::domain::SensorReading r{};
    r.id         = gh::domain::SensorChannelId{gh::domain::kSensorChannelIdAir};
    r.kind       = gh::domain::SensorKind::Air;
    r.read_at_ms = air.value.timestamp_ms;
    r.values.air = air.value;
    return R::success(r);
}

gh::domain::ErrorCode AM2315CSensor::resetQuirkIfNeeded_() noexcept {
    auto status = readStatus_();
    if (!status.ok()) return status.err;
    if ((status.value & 0x18) == 0x18) return gh::domain::ErrorCode::Ok;

    if (auto err = resetOneRegister_(0x1B);
        err != gh::domain::ErrorCode::Ok) return err;
    if (auto err = resetOneRegister_(0x1C);
        err != gh::domain::ErrorCode::Ok) return err;
    if (auto err = resetOneRegister_(0x1E);
        err != gh::domain::ErrorCode::Ok) return err;
    delay(10);
    return gh::domain::ErrorCode::Ok;
}

gh::domain::ErrorCode AM2315CSensor::resetOneRegister_(uint8_t reg) noexcept {
    bus_.beginTransmission(address_);
    bus_.write(reg);
    bus_.write(static_cast<uint8_t>(0x00));
    bus_.write(static_cast<uint8_t>(0x00));
    if (bus_.endTransmission() != 0) return gh::domain::ErrorCode::I2cNack;
    delay(5);

    uint8_t value[3] = {0};
    if (bus_.requestFrom(address_, static_cast<uint8_t>(3)) != 3) {
        return gh::domain::ErrorCode::I2cTimeout;
    }
    for (uint8_t i = 0; i < 3; ++i) value[i] = static_cast<uint8_t>(bus_.read());
    delay(10);

    bus_.beginTransmission(address_);
    bus_.write(static_cast<uint8_t>(0xB0 | reg));
    bus_.write(value[1]);
    bus_.write(value[2]);
    if (bus_.endTransmission() != 0) return gh::domain::ErrorCode::I2cNack;
    delay(5);
    return gh::domain::ErrorCode::Ok;
}

gh::domain::ErrorCode AM2315CSensor::triggerMeasurement_() noexcept {
    bus_.beginTransmission(address_);
    bus_.write(static_cast<uint8_t>(0xAC));
    bus_.write(static_cast<uint8_t>(0x33));
    bus_.write(static_cast<uint8_t>(0x00));
    if (bus_.endTransmission() != 0) return gh::domain::ErrorCode::I2cNack;
    return gh::domain::ErrorCode::Ok;
}

gh::domain::Result<uint8_t> AM2315CSensor::readStatus_() noexcept {
    using R = gh::domain::Result<uint8_t>;
    if (bus_.requestFrom(address_, static_cast<uint8_t>(1)) != 1) {
        return R::failure(gh::domain::ErrorCode::I2cTimeout);
    }
    delay(1);
    return R::success(static_cast<uint8_t>(bus_.read()));
}

gh::domain::ErrorCode AM2315CSensor::waitNotBusy_(uint32_t timeout_ms) noexcept {
    const uint32_t start = millis();
    while ((millis() - start) < timeout_ms) {
        auto status = readStatus_();
        if (!status.ok()) return status.err;
        if ((status.value & 0x80) == 0) return gh::domain::ErrorCode::Ok;
        delay(kBusyPollIntervalMs);
    }
    return gh::domain::ErrorCode::I2cTimeout;
}

gh::domain::ErrorCode AM2315CSensor::readBytes_(uint8_t* buf, uint8_t len) noexcept {
    if (bus_.requestFrom(address_, len) != len) {
        return gh::domain::ErrorCode::I2cTimeout;
    }
    for (uint8_t i = 0; i < len; ++i) {
        buf[i] = static_cast<uint8_t>(bus_.read());
    }
    return gh::domain::ErrorCode::Ok;
}

uint8_t AM2315CSensor::crc8_(const uint8_t* data, uint8_t len) noexcept {
    uint8_t crc = 0xFF;
    while (len--) {
        crc = static_cast<uint8_t>(crc ^ *data++);
        for (uint8_t i = 0; i < 8; ++i) {
            if (crc & 0x80) {
                crc = static_cast<uint8_t>((crc << 1) ^ 0x31);
            } else {
                crc = static_cast<uint8_t>(crc << 1);
            }
        }
    }
    return crc;
}

gh::domain::SensorStatus AM2315CSensor::probe() noexcept {
    // Tolerant probe: attempt init then a status read.
    // NACK / timeout -> Absent; other errors -> Faulty.
    if (init() != gh::domain::ErrorCode::Ok) {
        status_ = gh::domain::SensorStatus::Absent;
        return status_;
    }
    auto s = readStatus_();
    if (!s.ok()) {
        status_ = (s.err == gh::domain::ErrorCode::I2cNack ||
                   s.err == gh::domain::ErrorCode::I2cTimeout)
                  ? gh::domain::SensorStatus::Absent
                  : gh::domain::SensorStatus::Faulty;
        return status_;
    }
    status_ = gh::domain::SensorStatus::Ok;
    return status_;
}

}
