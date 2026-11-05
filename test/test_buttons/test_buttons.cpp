#include <unity.h>

#include "buttons.h"

void setUp() {}
void tearDown() {}

using namespace leeloo::buttons;

// `true` = pin HIGH (released, pull-up), `false` = LOW (pressed).

void test_ignores_bounce_shorter_than_debounce_window() {
  Debouncer d;
  TEST_ASSERT_EQUAL(Edge::None, d.update(false, 0));
  TEST_ASSERT_EQUAL(Edge::None, d.update(true, 5));    // bounce back
  TEST_ASSERT_EQUAL(Edge::None, d.update(false, 10));
  TEST_ASSERT_EQUAL(Edge::None, d.update(false, 25));  // only 15 ms stable
}

void test_reports_press_after_stable_low() {
  Debouncer d;
  d.update(false, 0);
  TEST_ASSERT_EQUAL(Edge::Pressed, d.update(false, kDebounceMs));
}

void test_reports_press_only_once_while_held() {
  Debouncer d;
  d.update(false, 0);
  d.update(false, kDebounceMs);
  TEST_ASSERT_EQUAL(Edge::None, d.update(false, 500));
}

void test_reports_release_after_stable_high() {
  Debouncer d;
  d.update(false, 0);
  d.update(false, kDebounceMs);
  TEST_ASSERT_EQUAL(Edge::None, d.update(true, 100));
  TEST_ASSERT_EQUAL(Edge::Released, d.update(true, 100 + kDebounceMs));
}

void test_classifies_799ms_as_short() {
  TEST_ASSERT_EQUAL(Press::Short, classify(799));
}

void test_classifies_800ms_as_long() {
  TEST_ASSERT_EQUAL(Press::Long, classify(800));
}

void test_elapsed_is_correct_across_millis_overflow() {
  // 0xFFFFFFF0 -> 0x00000390 is 928 ms after wrap-around.
  TEST_ASSERT_EQUAL_UINT32(928, elapsed_ms(0xFFFFFFF0u, 0x00000390u));
  TEST_ASSERT_EQUAL(Press::Long, classify(elapsed_ms(0xFFFFFFF0u, 0x00000390u)));
}

void test_debounces_across_millis_overflow() {
  Debouncer d;
  const std::uint32_t start = 0xFFFFFFF6u;
  d.update(false, start);
  TEST_ASSERT_EQUAL(Edge::Pressed, d.update(false, start + kDebounceMs));
}

void test_is_pending_until_debounce_window_elapses() {
  Debouncer d;
  d.update(false, 0);
  TEST_ASSERT_TRUE(d.pending());
  TEST_ASSERT_EQUAL(Edge::None, d.update(false, kDebounceMs - 1));
  TEST_ASSERT_TRUE(d.pending());
  TEST_ASSERT_EQUAL(Edge::Pressed, d.update(false, kDebounceMs));
  TEST_ASSERT_FALSE(d.pending());
}

void test_is_not_pending_after_rejected_bounce() {
  Debouncer d;
  d.update(false, 0);
  d.update(true, 5);
  TEST_ASSERT_EQUAL(Edge::None, d.update(true, 5 + kDebounceMs));
  TEST_ASSERT_FALSE(d.pending());
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_ignores_bounce_shorter_than_debounce_window);
  RUN_TEST(test_reports_press_after_stable_low);
  RUN_TEST(test_reports_press_only_once_while_held);
  RUN_TEST(test_reports_release_after_stable_high);
  RUN_TEST(test_classifies_799ms_as_short);
  RUN_TEST(test_classifies_800ms_as_long);
  RUN_TEST(test_elapsed_is_correct_across_millis_overflow);
  RUN_TEST(test_debounces_across_millis_overflow);
  RUN_TEST(test_is_pending_until_debounce_window_elapses);
  RUN_TEST(test_is_not_pending_after_rejected_bounce);
  return UNITY_END();
}
