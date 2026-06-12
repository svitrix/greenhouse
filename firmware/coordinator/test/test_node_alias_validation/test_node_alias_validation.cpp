#include <unity.h>
#include <string_view>
#include "ports/INodeAliasStore.hpp"

using gh::domain::isValidAlias;
using gh::domain::kMaxAliasBytes;

void test_accepts_plain_ascii() {
    TEST_ASSERT_TRUE(isValidAlias("Greenhouse 1"));
}

void test_accepts_valid_utf8_multibyte() {
    // "Café" (é = U+00E9, 2-byte) + "日" (U+65E5, 3-byte) + "🌱" (U+1F331, 4-byte)
    TEST_ASSERT_TRUE(isValidAlias("Caf\xC3\xA9 \xE6\x97\xA5 \xF0\x9F\x8C\xB1"));
}

void test_rejects_empty() {
    TEST_ASSERT_FALSE(isValidAlias(""));
}

void test_rejects_too_long() {
    char buf[kMaxAliasBytes + 2];
    for (auto& ch : buf) ch = 'x';
    buf[sizeof(buf) - 1] = '\0';
    TEST_ASSERT_FALSE(isValidAlias(std::string_view{buf, kMaxAliasBytes + 1}));
}

void test_rejects_control_chars() {
    TEST_ASSERT_FALSE(isValidAlias("bad\nname"));   // LF
    TEST_ASSERT_FALSE(isValidAlias("bad\tname"));   // TAB
    TEST_ASSERT_FALSE(isValidAlias(std::string_view{"a\0b", 3}));  // embedded NUL
    TEST_ASSERT_FALSE(isValidAlias("bad\x7F"));     // DEL
}

void test_rejects_truncated_utf8() {
    TEST_ASSERT_FALSE(isValidAlias("\xC3"));        // lead byte, no continuation
    TEST_ASSERT_FALSE(isValidAlias("ab\xE6\x97"));  // 3-byte seq cut short
}

void test_rejects_stray_continuation_byte() {
    TEST_ASSERT_FALSE(isValidAlias("\x80x"));
    TEST_ASSERT_FALSE(isValidAlias("\xBFx"));
}

void test_rejects_overlong_and_surrogate() {
    TEST_ASSERT_FALSE(isValidAlias("\xC0\xAF"));        // overlong '/'
    TEST_ASSERT_FALSE(isValidAlias("\xED\xA0\x80"));    // UTF-16 surrogate U+D800
    TEST_ASSERT_FALSE(isValidAlias("\xF5\x80\x80\x80")); // > U+10FFFF
}

void test_rejects_c1_control() {
    TEST_ASSERT_FALSE(isValidAlias("\xC2\x85"));   // NEL (U+0085)
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_accepts_plain_ascii);
    RUN_TEST(test_accepts_valid_utf8_multibyte);
    RUN_TEST(test_rejects_empty);
    RUN_TEST(test_rejects_too_long);
    RUN_TEST(test_rejects_control_chars);
    RUN_TEST(test_rejects_truncated_utf8);
    RUN_TEST(test_rejects_stray_continuation_byte);
    RUN_TEST(test_rejects_overlong_and_surrogate);
    RUN_TEST(test_rejects_c1_control);
    return UNITY_END();
}
