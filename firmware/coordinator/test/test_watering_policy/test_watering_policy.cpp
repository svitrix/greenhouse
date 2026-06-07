#include <unity.h>
#include "policies/WateringPolicy.hpp"
#include "entities/SoilSample.hpp"

using gh::domain::WateringPolicy;
using gh::domain::SoilSample;
using gh::domain::WateringDecision;

void test_stub_always_returns_no_action() {
    WateringPolicy policy;
    SoilSample dry{0, 35000, 0, 200};
    SoilSample wet{0, 12000, 100, 200};
    TEST_ASSERT_EQUAL(static_cast<int>(WateringDecision::NoAction),
                      static_cast<int>(policy.decide(dry)));
    TEST_ASSERT_EQUAL(static_cast<int>(WateringDecision::NoAction),
                      static_cast<int>(policy.decide(wet)));
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_stub_always_returns_no_action);
    return UNITY_END();
}
