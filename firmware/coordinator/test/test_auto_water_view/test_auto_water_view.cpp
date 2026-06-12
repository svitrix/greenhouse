#include <unity.h>
#include <ArduinoJson.h>
#include <string>
#include "AutoWaterView.hpp"

using gh::presentation::AutoWaterView;
using gh::app::AutoWaterDecision;
using gh::app::AutoWaterOutcome;
using gh::domain::NodeId;

void test_auto_water_view_null_moisture_when_absent(void) {
    AutoWaterDecision d{};
    d.outcome      = AutoWaterOutcome::Disabled;
    d.monotonic_ms = 42'000;

    JsonDocument doc;
    AutoWaterView::build(d, doc.to<JsonObject>());

    TEST_ASSERT_TRUE(doc["avg_moisture_pct"].isNull());
    TEST_ASSERT_EQUAL_STRING("disabled", doc["last_decision"].as<const char*>());
    TEST_ASSERT_EQUAL_UINT(42'000, doc["last_decision_ms"].as<uint32_t>());
    TEST_ASSERT_EQUAL_UINT(0, doc["fresh_sources"].as<JsonArray>().size());
    TEST_ASSERT_EQUAL_UINT(0, doc["stale_sources"].as<JsonArray>().size());
}

void test_auto_water_view_serialises_sources(void) {
    AutoWaterDecision d{};
    d.outcome          = AutoWaterOutcome::Started;
    d.avg_moisture_pct = 18.5f;
    d.monotonic_ms     = 1'000;
    d.fresh_sources.push_back(NodeId{0x00124B001A2B3C4Dull});
    d.stale_sources.push_back(NodeId{0x00124B00DEADBEEFull});

    JsonDocument doc;
    AutoWaterView::build(d, doc.to<JsonObject>());

    TEST_ASSERT_EQUAL_FLOAT(18.5f, doc["avg_moisture_pct"].as<float>());
    TEST_ASSERT_EQUAL_STRING("00124B001A2B3C4D",
        doc["fresh_sources"][0].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("00124B00DEADBEEF",
        doc["stale_sources"][0].as<const char*>());
}

// The IEEE strings come from stack-locals inside build(); confirm they survive
// into a serializeJson() made after build()'s scope (JsonString copy path).
void test_auto_water_view_strings_survive_scope(void) {
    JsonDocument doc;
    {
        AutoWaterDecision d{};
        d.fresh_sources.push_back(NodeId{0x00124B001A2B3C4Dull});
        AutoWaterView::build(d, doc.to<JsonObject>());
    }
    volatile char scratch[64];
    for (auto& c : scratch) c = static_cast<char>(0xAB);
    (void)scratch;

    std::string out;
    serializeJson(doc, out);
    TEST_ASSERT_NOT_EQUAL(std::string::npos, out.find("\"00124B001A2B3C4D\""));
}

void setUp() {}
void tearDown() {}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_auto_water_view_null_moisture_when_absent);
    RUN_TEST(test_auto_water_view_serialises_sources);
    RUN_TEST(test_auto_water_view_strings_survive_scope);
    return UNITY_END();
}
