#include <Arduino.h>
#include <Wire.h>
#include <unity.h>
#include "drivers/ChirpSoilSensor.hpp"
#include "AppConfig.hpp"

using gh::domain::ErrorCode;
using gh::infra::ChirpSoilSensor;
namespace cfg = gh::app::AppConfig;

namespace {
ChirpSoilSensor* g_sensor = nullptr;
}

void test_init_succeeds_on_known_good_sensor() {
    TEST_ASSERT_EQUAL(static_cast<int>(ErrorCode::Ok),
                      static_cast<int>(g_sensor->init()));
}

void test_read_returns_capacitance_in_air_range() {
    TEST_ASSERT_EQUAL(static_cast<int>(ErrorCode::Ok),
                      static_cast<int>(g_sensor->init()));
    auto result = g_sensor->readSoil();
    TEST_ASSERT_TRUE(result.ok());
    // In free air: README documents 290..310 at 5V; 3V3 typically similar.
    // Allow generous range to absorb sensor variation and ambient humidity.
    TEST_ASSERT_GREATER_THAN_UINT16(200, result.value.raw_capacitance);
    TEST_ASSERT_LESS_THAN_UINT16(500, result.value.raw_capacitance);
}

void test_read_returns_temperature_in_room_range() {
    TEST_ASSERT_EQUAL(static_cast<int>(ErrorCode::Ok),
                      static_cast<int>(g_sensor->init()));
    auto result = g_sensor->readSoil();
    TEST_ASSERT_TRUE(result.ok());
    // Room temperature: 10°C..40°C → 100..400 in 0.1°C units.
    TEST_ASSERT_GREATER_THAN_INT16(100, result.value.temperature_c_x10);
    TEST_ASSERT_LESS_THAN_INT16(400, result.value.temperature_c_x10);
}

void test_read_without_init_returns_SensorNotReady() {
    ChirpSoilSensor uninitialised(Wire, cfg::kChirpAddress);
    auto result = uninitialised.readSoil();
    TEST_ASSERT_FALSE(result.ok());
    TEST_ASSERT_EQUAL(static_cast<int>(ErrorCode::SensorNotReady),
                      static_cast<int>(result.err));
}

void test_init_on_wrong_address_returns_I2cNack() {
    ChirpSoilSensor wrongAddr(Wire, 0x70);  // unused address
    TEST_ASSERT_EQUAL(static_cast<int>(ErrorCode::I2cNack),
                      static_cast<int>(wrongAddr.init()));
}

void setup() {
    delay(2000);
    Wire.begin(cfg::kI2cSdaPin, cfg::kI2cSclPin, cfg::kI2cFrequencyHz);
    static ChirpSoilSensor sensor(Wire, cfg::kChirpAddress);
    g_sensor = &sensor;

    UNITY_BEGIN();
    RUN_TEST(test_init_succeeds_on_known_good_sensor);
    RUN_TEST(test_read_returns_capacitance_in_air_range);
    RUN_TEST(test_read_returns_temperature_in_room_range);
    RUN_TEST(test_read_without_init_returns_SensorNotReady);
    RUN_TEST(test_init_on_wrong_address_returns_I2cNack);
    UNITY_END();
}

void loop() {}
