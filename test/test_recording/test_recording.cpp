#include <unity.h>

#include <array>

#include "recording.h"

using namespace leeloo::recording;

void setUp() {}
void tearDown() {}

void test_selects_two_minutes_at_eight_kilohertz_with_reserve() {
  const Format f = choose_format(kReserveBytes + kWriteSlackBytes + 1920044);
  TEST_ASSERT_EQUAL_UINT32(8000, f.sample_rate);
  TEST_ASSERT_EQUAL_UINT32(120, f.max_seconds);
  TEST_ASSERT_EQUAL_UINT32(1920044, f.max_wav_bytes());
}

void test_falls_back_to_thirty_seconds_at_sixteen_kilohertz() {
  const Format f = choose_format(kReserveBytes + kWriteSlackBytes + 960044);
  TEST_ASSERT_EQUAL_UINT32(16000, f.sample_rate);
  TEST_ASSERT_EQUAL_UINT32(30, f.max_seconds);
  TEST_ASSERT_EQUAL_UINT32(960044, f.max_wav_bytes());
}

void test_rejects_space_below_reserve_plus_short_recording() {
  const Format f = choose_format(kReserveBytes + kWriteSlackBytes + 960043);
  TEST_ASSERT_EQUAL_UINT32(0, f.sample_rate);
}

void test_builds_and_validates_a_pcm_mono_wav_header() {
  std::array<std::uint8_t, kWavHeaderBytes> header{};
  make_wav_header(header.data(), 8000, 1600);
  TEST_ASSERT_TRUE(valid_wav_header(header.data(), kWavHeaderBytes + 1600));
  TEST_ASSERT_FALSE(valid_wav_header(header.data(), kWavHeaderBytes + 1598));
}

void test_converts_i2s_samples_to_signed_pcm16() {
  TEST_ASSERT_EQUAL_INT16(1234, sample_to_pcm16(1234L << 16));
  TEST_ASSERT_EQUAL_INT16(-1234, sample_to_pcm16(-1234L * 65536));
  TEST_ASSERT_EQUAL_INT16(32767, sample_to_pcm16(INT32_MAX));
  TEST_ASSERT_EQUAL_INT16(-32768, sample_to_pcm16(INT32_MIN));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_selects_two_minutes_at_eight_kilohertz_with_reserve);
  RUN_TEST(test_falls_back_to_thirty_seconds_at_sixteen_kilohertz);
  RUN_TEST(test_rejects_space_below_reserve_plus_short_recording);
  RUN_TEST(test_builds_and_validates_a_pcm_mono_wav_header);
  RUN_TEST(test_converts_i2s_samples_to_signed_pcm16);
  return UNITY_END();
}
