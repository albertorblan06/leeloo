#include <unity.h>

#include "leeloo_core.h"

void setUp() {}
void tearDown() {}

void test_utf8_sequence_length_is_one_for_ascii() {
  TEST_ASSERT_EQUAL_INT(1, leeloo::utf8_sequence_length('a'));
  TEST_ASSERT_EQUAL_INT(1, leeloo::utf8_sequence_length(0x00));
  TEST_ASSERT_EQUAL_INT(1, leeloo::utf8_sequence_length(0x7F));
}

void test_utf8_sequence_length_is_two_for_spanish_accented_letters() {
  // U+00F1 'n with tilde' is C3 B1 in UTF-8.
  TEST_ASSERT_EQUAL_INT(2, leeloo::utf8_sequence_length(0xC3));
}

void test_utf8_sequence_length_covers_three_and_four_byte_leads() {
  TEST_ASSERT_EQUAL_INT(3, leeloo::utf8_sequence_length(0xE2));
  TEST_ASSERT_EQUAL_INT(4, leeloo::utf8_sequence_length(0xF0));
}

void test_utf8_sequence_length_rejects_continuation_and_invalid_bytes() {
  TEST_ASSERT_EQUAL_INT(0, leeloo::utf8_sequence_length(0x80));  // continuation
  TEST_ASSERT_EQUAL_INT(0, leeloo::utf8_sequence_length(0xBF));
  TEST_ASSERT_EQUAL_INT(0, leeloo::utf8_sequence_length(0xC0));  // overlong
  TEST_ASSERT_EQUAL_INT(0, leeloo::utf8_sequence_length(0xC1));
  TEST_ASSERT_EQUAL_INT(0, leeloo::utf8_sequence_length(0xF5));  // > U+10FFFF
  TEST_ASSERT_EQUAL_INT(0, leeloo::utf8_sequence_length(0xFF));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_utf8_sequence_length_is_one_for_ascii);
  RUN_TEST(test_utf8_sequence_length_is_two_for_spanish_accented_letters);
  RUN_TEST(test_utf8_sequence_length_covers_three_and_four_byte_leads);
  RUN_TEST(test_utf8_sequence_length_rejects_continuation_and_invalid_bytes);
  return UNITY_END();
}
