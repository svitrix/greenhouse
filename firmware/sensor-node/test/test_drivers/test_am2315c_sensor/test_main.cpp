#include <Arduino.h>
#include <Wire.h>
#include <unity.h>
#include <AM2315C.h>
#include "drivers/AM2315CSensor.hpp"
#include "AppConfig.hpp"

using gh::domain::ErrorCode;
using gh::infra::AM2315CSensor;
namespace cfg = gh::app::AppConfig;

namespace {
AM2315CSensor* g_sensor = nullptr;
AM2315C*       g_tillaart = nullptr;
}

void test_init_succeeds_on_known_good_sensor() {
    TEST_ASSERT_EQUAL(static_cast<int>(ErrorCode::Ok),
                      static_cast<int>(g_sensor->init()));
}

void test_read_returns_temperature_in_room_range() {
    TEST_ASSERT_EQUAL(static_cast<int>(ErrorCode::Ok),
                      static_cast<int>(g_sensor->init()));
    delay(1100);
    auto result = g_sensor->readAir();
    TEST_ASSERT_TRUE(result.ok());
    // Room: 10..40 °C → 100..400 in 0.1°C units
    TEST_ASSERT_GREATER_THAN_INT16(100, result.value.temperature_c_x10);
    TEST_ASSERT_LESS_THAN_INT16(400, result.value.temperature_c_x10);
}

void test_read_returns_humidity_in_room_range() {
    TEST_ASSERT_EQUAL(static_cast<int>(ErrorCode::Ok),
                      static_cast<int>(g_sensor->init()));
    delay(1100);
    auto result = g_sensor->readAir();
    TEST_ASSERT_TRUE(result.ok());
    // Room: 10..90 %RH → 100..900 in 0.1% units
    TEST_ASSERT_GREATER_THAN_UINT16(100, result.value.humidity_pct_x10);
    TEST_ASSERT_LESS_THAN_UINT16(900, result.value.humidity_pct_x10);
}

void test_rate_limit_returns_SensorTooFast() {
    TEST_ASSERT_EQUAL(static_cast<int>(ErrorCode::Ok),
                      static_cast<int>(g_sensor->init()));
    delay(1100);
    auto first = g_sensor->readAir();
    TEST_ASSERT_TRUE(first.ok());
    auto second = g_sensor->readAir();
    TEST_ASSERT_FALSE(second.ok());
    TEST_ASSERT_EQUAL(static_cast<int>(ErrorCode::SensorTooFast),
                      static_cast<int>(second.err));
}

void test_read_without_init_returns_SensorNotReady() {
    AM2315CSensor uninitialised(Wire);
    auto result = uninitialised.readAir();
    TEST_ASSERT_FALSE(result.ok());
    TEST_ASSERT_EQUAL(static_cast<int>(ErrorCode::SensorNotReady),
                      static_cast<int>(result.err));
}

void test_cross_validation_with_tillaart() {
    TEST_ASSERT_EQUAL(static_cast<int>(ErrorCode::Ok),
                      static_cast<int>(g_sensor->init()));
    TEST_ASSERT_TRUE(g_tillaart->begin());

    delay(1100);   // satisfy our 1Hz rate limit
    auto our = g_sensor->readAir();
    TEST_ASSERT_TRUE(our.ok());

    delay(1100);   // satisfy Tillaart's 1Hz rate limit
    int rc = g_tillaart->read();
    TEST_ASSERT_EQUAL_INT(0, rc);

    const float their_temp = g_tillaart->getTemperature();
    const float their_hum  = g_tillaart->getHumidity();
    const float our_temp = our.value.temperature_c_x10 / 10.0f;
    const float our_hum  = our.value.humidity_pct_x10  / 10.0f;

    char msg[160];
    snprintf(msg, sizeof(msg),
             "TEMP us=%.2f tillaart=%.2f / RH us=%.2f tillaart=%.2f",
             static_cast<double>(our_temp), static_cast<double>(their_temp),
             static_cast<double>(our_hum), static_cast<double>(their_hum));
    Serial.println(msg);

    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.5f, their_temp, our_temp, msg);
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(1.0f, their_hum, our_hum, msg);
}

void setup() {
    delay(2000);
    Serial.begin(115200);
    Wire.begin(cfg::kI2cSdaPin, cfg::kI2cSclPin, cfg::kI2cFrequencyHz);

    static AM2315CSensor sensor(Wire);
    static AM2315C       tillaart(&Wire);
    g_sensor   = &sensor;
    g_tillaart = &tillaart;

    UNITY_BEGIN();
    RUN_TEST(test_init_succeeds_on_known_good_sensor);
    RUN_TEST(test_read_returns_temperature_in_room_range);
    RUN_TEST(test_read_returns_humidity_in_room_range);
    RUN_TEST(test_rate_limit_returns_SensorTooFast);
    RUN_TEST(test_read_without_init_returns_SensorNotReady);
    RUN_TEST(test_cross_validation_with_tillaart);
    UNITY_END();
}

void loop() {}
