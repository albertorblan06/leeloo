#include <unity.h>

#include "settings.h"

using namespace leeloo::settings;

void setUp() {}
void tearDown() {}

void test_defaults_match_device_menu_spec() {
  Settings s;
  TEST_ASSERT_EQUAL(BrightnessMode::Manual, s.brightness_mode);
  TEST_ASSERT_EQUAL_UINT8(100, s.brightness_percent);
  TEST_ASSERT_EQUAL_UINT8(100, s.contrast_percent);
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
  TEST_ASSERT_EQUAL_UINT8(255, display_register(100, 100));
}

void test_register_scales_by_both_percentages() {
  TEST_ASSERT_EQUAL_UINT8(128, display_register(50, 100));  // 127.5 rounds up
  TEST_ASSERT_EQUAL_UINT8(64, display_register(50, 50));    // 63.75 rounds up
}

void test_register_never_reaches_zero_at_minimum() {
  TEST_ASSERT_EQUAL_UINT8(3, display_register(10, 10));  // 2.55 rounds up
}

void test_wifi_cycles_through_three_modes_and_wraps() {
  TEST_ASSERT_EQUAL(WifiMode::On, cycle_wifi(WifiMode::Auto, +1));
  TEST_ASSERT_EQUAL(WifiMode::Off, cycle_wifi(WifiMode::On, +1));
  TEST_ASSERT_EQUAL(WifiMode::On, cycle_wifi(WifiMode::Off, -1));
  TEST_ASSERT_EQUAL(WifiMode::Auto, cycle_wifi(WifiMode::On, -1));
}

void test_brightness_mode_toggles() {
  TEST_ASSERT_EQUAL(BrightnessMode::Auto, cycle_brightness(BrightnessMode::Manual));
  TEST_ASSERT_EQUAL(BrightnessMode::Manual, cycle_brightness(BrightnessMode::Auto));
}

void test_sanitize_keeps_valid_values() {
  Settings s;
  s.brightness_percent = 40;
  s.contrast_percent = 75;
  s.wifi = WifiMode::On;
  const Settings out = sanitize(s);
  TEST_ASSERT_EQUAL_UINT8(40, out.brightness_percent);
  TEST_ASSERT_EQUAL_UINT8(75, out.contrast_percent);
  TEST_ASSERT_EQUAL(WifiMode::On, out.wifi);
}

void test_sanitize_clamps_out_of_range_percentages_to_the_step_grid() {
  Settings s;
  s.brightness_percent = 0;
  s.contrast_percent = 255;
  const Settings out = sanitize(s);
  TEST_ASSERT_EQUAL_UINT8(kMinPercent, out.brightness_percent);
  TEST_ASSERT_EQUAL_UINT8(kMaxPercent, out.contrast_percent);
}

void test_sanitize_rounds_to_the_nearest_five_percent() {
  Settings s;
  s.brightness_percent = 47;
  s.contrast_percent = 52;
  const Settings out = sanitize(s);
  TEST_ASSERT_EQUAL_UINT8(45, out.brightness_percent);
  TEST_ASSERT_EQUAL_UINT8(50, out.contrast_percent);
}

void test_sanitize_replaces_unknown_wifi_value_with_default() {
  Settings s;
  s.wifi = static_cast<WifiMode>(7);
  TEST_ASSERT_EQUAL(WifiMode::Auto, sanitize(s).wifi);
}

void test_sanitize_replaces_unknown_brightness_mode_with_default() {
  Settings s;
  s.brightness_mode = static_cast<BrightnessMode>(9);
  TEST_ASSERT_EQUAL(BrightnessMode::Manual, sanitize(s).brightness_mode);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_sanitize_replaces_unknown_brightness_mode_with_default);
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
  RUN_TEST(test_register_scales_by_both_percentages);
  RUN_TEST(test_register_never_reaches_zero_at_minimum);
  RUN_TEST(test_wifi_cycles_through_three_modes_and_wraps);
  RUN_TEST(test_brightness_mode_toggles);
  return UNITY_END();
}
