#include <unity.h>
#include "errors/ErrorCode.hpp"
#include "util/Result.hpp"

using gh::domain::ErrorCode;
using gh::domain::Result;

void test_success_is_ok() {
    auto r = Result<int>::success(42);
    TEST_ASSERT_TRUE(r.ok());
    TEST_ASSERT_EQUAL_INT(42, r.value);
    TEST_ASSERT_EQUAL(static_cast<int>(ErrorCode::Ok), static_cast<int>(r.err));
}

void test_failure_is_not_ok() {
    auto r = Result<int>::failure(ErrorCode::I2cTimeout);
    TEST_ASSERT_FALSE(r.ok());
    TEST_ASSERT_EQUAL(static_cast<int>(ErrorCode::I2cTimeout), static_cast<int>(r.err));
}

void test_ok_only_when_err_is_ok() {
    auto r1 = Result<int>{ErrorCode::Ok, 0};
    auto r2 = Result<int>{ErrorCode::Unknown, 0};
    TEST_ASSERT_TRUE(r1.ok());
    TEST_ASSERT_FALSE(r2.ok());
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_success_is_ok);
    RUN_TEST(test_failure_is_not_ok);
    RUN_TEST(test_ok_only_when_err_is_ok);
    return UNITY_END();
}
