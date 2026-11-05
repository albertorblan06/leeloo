#include <unity.h>

#include "joystick.h"

void setUp() {}
void tearDown() {}

using namespace leeloo::joystick;

void test_ignores_values_inside_dead_zone() {
  Axis axis(2048);
  TEST_ASSERT_EQUAL(Direction::None, axis.update(2048));
  TEST_ASSERT_EQUAL(Direction::None, axis.update(2048 + kEnterDelta - 1));
  TEST_ASSERT_EQUAL(Direction::None, axis.update(2048 - kEnterDelta + 1));
}

void test_reports_high_at_enter_threshold() {
  Axis axis(2048);
  TEST_ASSERT_EQUAL(Direction::High, axis.update(2048 + kEnterDelta));
}

void test_reports_low_at_enter_threshold() {
  Axis axis(2048);
  TEST_ASSERT_EQUAL(Direction::Low, axis.update(2048 - kEnterDelta));
}

void test_reports_direction_once_while_held() {
  Axis axis(2048);
  axis.update(4095);
  TEST_ASSERT_EQUAL(Direction::None, axis.update(4095));
  TEST_ASSERT_EQUAL(Direction::None, axis.update(3500));
}

void test_hysteresis_does_not_retrigger_between_exit_and_enter() {
  Axis axis(2048);
  axis.update(4095);                                      // High
  axis.update(2048 + kExitDelta + 10);                    // still held, above exit
  TEST_ASSERT_EQUAL(Direction::None, axis.update(2048 + kEnterDelta));
}

void test_rearms_after_returning_to_center() {
  Axis axis(2048);
  axis.update(4095);
  axis.update(2048);  // back inside the exit band
  TEST_ASSERT_EQUAL(Direction::High, axis.update(4095));
}

void test_swing_from_high_to_low_reports_low() {
  Axis axis(2048);
  axis.update(4095);
  TEST_ASSERT_EQUAL(Direction::Low, axis.update(0));
}

void test_uses_custom_center() {
  Axis axis(1900);
  TEST_ASSERT_EQUAL(Direction::None, axis.update(1900 + kEnterDelta - 1));
  TEST_ASSERT_EQUAL(Direction::High, axis.update(1900 + kEnterDelta));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_ignores_values_inside_dead_zone);
  RUN_TEST(test_reports_high_at_enter_threshold);
  RUN_TEST(test_reports_low_at_enter_threshold);
  RUN_TEST(test_reports_direction_once_while_held);
  RUN_TEST(test_hysteresis_does_not_retrigger_between_exit_and_enter);
  RUN_TEST(test_rearms_after_returning_to_center);
  RUN_TEST(test_swing_from_high_to_low_reports_low);
  RUN_TEST(test_uses_custom_center);
  return UNITY_END();
}
