#include <unity.h>

#include "settings.h"

using namespace leeloo::settings;

void setUp() {}
void tearDown() {}

void test_defaults_match_device_menu_spec() {
  Settings s;
  TEST_ASSERT_EQUAL_UINT8(100, s.brightness_percent);
  TEST_ASSERT_EQUAL(WifiMode::Auto, s.wifi);
}

void test_step_moves_by_five_percent() {
  TEST_ASSERT_EQUAL_UINT8(55, step_percent(50, +1));
  TEST_ASSERT_EQUAL_UINT8(45, step_percent(50, -1));
}

void test_step_clamps_at_minimum_ten_percent() {
  TEST_ASSERT_EQUAL_UINT8(10, step_percent(10, -1));
  TEST_ASSERT_EQUAL_UINT8(10, step_percent(12, -1));
}

void test_step_clamps_at_maximum_hundred_percent() {
  TEST_ASSERT_EQUAL_UINT8(100, step_percent(100, +1));
  TEST_ASSERT_EQUAL_UINT8(100, step_percent(98, +1));
}

void test_step_ignores_values_outside_range() {
  TEST_ASSERT_EQUAL_UINT8(10, step_percent(0, +1));
  TEST_ASSERT_EQUAL_UINT8(100, step_percent(255, -1));
}

void test_register_is_full_scale_at_hundred_percent() {
  TEST_ASSERT_EQUAL_UINT8(255, display_register(100));
}

void test_register_scales_linearly_with_the_percentage() {
  TEST_ASSERT_EQUAL_UINT8(128, display_register(50));  // 127.5 rounds up
  TEST_ASSERT_EQUAL_UINT8(64, display_register(25));   // 63.75 rounds up
}

void test_register_never_reaches_zero_at_minimum() {
  TEST_ASSERT_EQUAL_UINT8(26, display_register(kMinPercent));  // 25.5 rounds up; never 0 (useful range: hardware check)
}

void test_register_is_monotonic_over_the_whole_range() {
  std::uint8_t previous = 0;
  for (int p = kMinPercent; p <= kMaxPercent; p += kStepPercent) {
    const std::uint8_t now = display_register(static_cast<std::uint8_t>(p));
    TEST_ASSERT_TRUE(now > previous);
    previous = now;
  }
}

void test_wifi_cycles_through_three_modes_and_wraps() {
  TEST_ASSERT_EQUAL(WifiMode::On, cycle_wifi(WifiMode::Auto, +1));
  TEST_ASSERT_EQUAL(WifiMode::Off, cycle_wifi(WifiMode::On, +1));
  TEST_ASSERT_EQUAL(WifiMode::On, cycle_wifi(WifiMode::Off, -1));
  TEST_ASSERT_EQUAL(WifiMode::Auto, cycle_wifi(WifiMode::On, -1));
}

void test_sanitize_keeps_valid_values() {
  Settings s;
  s.brightness_percent = 40;
  s.wifi = WifiMode::On;
  const Settings out = sanitize(s);
  TEST_ASSERT_EQUAL_UINT8(40, out.brightness_percent);
  TEST_ASSERT_EQUAL(WifiMode::On, out.wifi);
}

void test_sanitize_clamps_out_of_range_percentages_to_the_step_grid() {
  Settings s;
  s.brightness_percent = 0;
  TEST_ASSERT_EQUAL_UINT8(kMinPercent, sanitize(s).brightness_percent);
  s.brightness_percent = 255;
  TEST_ASSERT_EQUAL_UINT8(kMaxPercent, sanitize(s).brightness_percent);
}

void test_sanitize_rounds_to_the_nearest_five_percent() {
  Settings s;
  s.brightness_percent = 47;
  TEST_ASSERT_EQUAL_UINT8(45, sanitize(s).brightness_percent);
  s.brightness_percent = 52;
  TEST_ASSERT_EQUAL_UINT8(50, sanitize(s).brightness_percent);
}

void test_sanitize_replaces_unknown_wifi_value_with_default() {
  Settings s;
  s.wifi = static_cast<WifiMode>(7);
  TEST_ASSERT_EQUAL(WifiMode::Auto, sanitize(s).wifi);
}

void test_reading_step_defaults_to_25_ppm() {
  TEST_ASSERT_EQUAL_UINT16(25, Settings{}.reading_step_ppm);
}

void test_reading_step_moves_through_the_supported_steps_and_clamps() {
  TEST_ASSERT_EQUAL_UINT16(50, adjust_reading_step(25, +1));
  TEST_ASSERT_EQUAL_UINT16(100, adjust_reading_step(50, +1));
  TEST_ASSERT_EQUAL_UINT16(100, adjust_reading_step(100, +1));
  TEST_ASSERT_EQUAL_UINT16(10, adjust_reading_step(25, -1));
  TEST_ASSERT_EQUAL_UINT16(5, adjust_reading_step(10, -1));
  TEST_ASSERT_EQUAL_UINT16(5, adjust_reading_step(5, -1));
}

void test_reading_step_with_an_invalid_value_restarts_from_the_default() {
  TEST_ASSERT_EQUAL_UINT16(50, adjust_reading_step(7, +1));
  TEST_ASSERT_EQUAL_UINT16(10, adjust_reading_step(0, -1));
}

void test_sanitize_replaces_an_unsupported_reading_step_with_the_default() {
  Settings s;
  s.reading_step_ppm = 33;
  TEST_ASSERT_EQUAL_UINT16(25, sanitize(s).reading_step_ppm);
  s.reading_step_ppm = 100;
  TEST_ASSERT_EQUAL_UINT16(100, sanitize(s).reading_step_ppm);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_sanitize_keeps_valid_values);
  RUN_TEST(test_sanitize_clamps_out_of_range_percentages_to_the_step_grid);
  RUN_TEST(test_sanitize_rounds_to_the_nearest_five_percent);
  RUN_TEST(test_sanitize_replaces_unknown_wifi_value_with_default);
  RUN_TEST(test_defaults_match_device_menu_spec);
  RUN_TEST(test_step_moves_by_five_percent);
  RUN_TEST(test_step_clamps_at_minimum_ten_percent);
  RUN_TEST(test_step_clamps_at_maximum_hundred_percent);
  RUN_TEST(test_step_ignores_values_outside_range);
  RUN_TEST(test_register_is_full_scale_at_hundred_percent);
  RUN_TEST(test_register_scales_linearly_with_the_percentage);
  RUN_TEST(test_register_is_monotonic_over_the_whole_range);
  RUN_TEST(test_register_never_reaches_zero_at_minimum);
  RUN_TEST(test_wifi_cycles_through_three_modes_and_wraps);
  RUN_TEST(test_reading_step_defaults_to_25_ppm);
  RUN_TEST(test_reading_step_moves_through_the_supported_steps_and_clamps);
  RUN_TEST(test_reading_step_with_an_invalid_value_restarts_from_the_default);
  RUN_TEST(test_sanitize_replaces_an_unsupported_reading_step_with_the_default);
  return UNITY_END();
}
