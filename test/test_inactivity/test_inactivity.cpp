#include <unity.h>

#include "inactivity.h"

using namespace leeloo::upload;

void setUp() {}
void tearDown() {}

void test_stopped_timer_never_expires() {
  InactivityTimer t(1000);
  TEST_ASSERT_FALSE(t.expired(999999));
}

void test_does_not_expire_before_the_limit() {
  InactivityTimer t(1000);
  t.start(0);
  TEST_ASSERT_FALSE(t.expired(999));
}

void test_expires_exactly_at_the_limit() {
  InactivityTimer t(1000);
  t.start(0);
  TEST_ASSERT_TRUE(t.expired(1000));
}

void test_activity_restarts_the_window() {
  InactivityTimer t(1000);
  t.start(0);
  t.touch(900);
  TEST_ASSERT_FALSE(t.expired(1500));
  TEST_ASSERT_TRUE(t.expired(1900));
}

void test_stop_disarms_the_timer() {
  InactivityTimer t(1000);
  t.start(0);
  t.stop();
  TEST_ASSERT_FALSE(t.expired(5000));
}

void test_window_is_correct_across_millis_wraparound() {
  InactivityTimer t(1000);
  const std::uint32_t almost_wrap = 0xFFFFFFFFu - 200;  // 200 ms before millis() wraps
  t.start(almost_wrap);
  TEST_ASSERT_FALSE(t.expired(almost_wrap + 999));
  TEST_ASSERT_TRUE(t.expired(almost_wrap + 1000));  // wraps to a small number
}

void test_default_upload_limit_is_five_minutes() {
  TEST_ASSERT_EQUAL_UINT32(5u * 60u * 1000u, kUploadInactivityMs);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_stopped_timer_never_expires);
  RUN_TEST(test_does_not_expire_before_the_limit);
  RUN_TEST(test_expires_exactly_at_the_limit);
  RUN_TEST(test_activity_restarts_the_window);
  RUN_TEST(test_stop_disarms_the_timer);
  RUN_TEST(test_window_is_correct_across_millis_wraparound);
  RUN_TEST(test_default_upload_limit_is_five_minutes);
  return UNITY_END();
}
