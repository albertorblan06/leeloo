#include <unity.h>

#include <cstring>

#include "recording.h"

using namespace leeloo::recording;

void setUp() {}
void tearDown() {}

namespace {
// Free space that is never recorded over: the reserve, the write margin and the WAV header.
constexpr std::size_t kKeep = kReserveBytes + kWriteSlackBytes + kWavHeaderBytes;
constexpr std::size_t kSecond = kBytesPerSecond;  // 32 000 bytes of 16 kHz mono 16-bit
constexpr std::uint32_t kMaxPcm = kSampleRate * kMaxSeconds * 2;  // 19 200 000
}  // namespace

// --- plan_format -----------------------------------------------------------------

void test_the_length_is_the_whole_seconds_that_fit_after_the_reserve() {
  const Format f = plan_format(kKeep + 100 * kSecond);
  TEST_ASSERT_EQUAL_UINT32(16000, f.sample_rate);
  TEST_ASSERT_EQUAL_UINT32(100, f.max_seconds);
  TEST_ASSERT_EQUAL_UINT32(100 * kSecond, f.max_pcm_bytes);
  TEST_ASSERT_EQUAL_UINT32(100 * kSecond + kWavHeaderBytes, f.max_wav_bytes());
}

void test_a_partial_second_is_not_promised() {
  TEST_ASSERT_EQUAL_UINT32(99, plan_format(kKeep + 100 * kSecond - 1).max_seconds);
  TEST_ASSERT_EQUAL_UINT32(100, plan_format(kKeep + 100 * kSecond + kSecond - 1).max_seconds);
}

void test_the_recording_plus_the_reserve_never_exceeds_the_free_space() {
  for (std::size_t free = 0; free < kKeep + 700 * kSecond; free += 12345) {
    const Format f = plan_format(free);
    if (f.sample_rate == 0) continue;
    TEST_ASSERT_TRUE(f.max_wav_bytes() + kReserveBytes + kWriteSlackBytes <= free);
  }
}

void test_the_minimum_is_five_seconds() {
  TEST_ASSERT_EQUAL_UINT32(5, plan_format(kKeep + 5 * kSecond).max_seconds);
  TEST_ASSERT_EQUAL_UINT32(0, plan_format(kKeep + 5 * kSecond - 1).sample_rate);  // 4 s: not worth it
}

void test_the_length_is_capped_at_ten_minutes() {
  TEST_ASSERT_EQUAL_UINT32(600, plan_format(kKeep + 600 * kSecond).max_seconds);
  TEST_ASSERT_EQUAL_UINT32(600, plan_format(kKeep + 100000 * kSecond).max_seconds);
  TEST_ASSERT_EQUAL_UINT32(kMaxPcm, plan_format(0x7FFFFFFF).max_pcm_bytes);
}

void test_no_free_space_fits_nothing() {
  TEST_ASSERT_EQUAL_UINT32(0, plan_format(0).sample_rate);
  TEST_ASSERT_EQUAL_UINT32(0, plan_format(kKeep).sample_rate);
  TEST_ASSERT_EQUAL_UINT32(0, plan_format(kReserveBytes / 2).sample_rate);  // below the reserve itself
}

// --- WAV header ------------------------------------------------------------------

namespace {
void header(std::uint8_t* out, std::uint32_t rate, std::uint32_t pcm) { make_wav_header(out, rate, pcm); }
}  // namespace

void test_header_is_44_bytes_of_mono_16_bit_pcm() {
  std::uint8_t h[kWavHeaderBytes];
  header(h, 8000, 1000);
  TEST_ASSERT_EQUAL_MEMORY("RIFF", h, 4);
  TEST_ASSERT_EQUAL_MEMORY("WAVEfmt ", h + 8, 8);
  TEST_ASSERT_EQUAL_MEMORY("data", h + 36, 4);
  TEST_ASSERT_EQUAL_UINT8(1, h[20]);   // PCM
  TEST_ASSERT_EQUAL_UINT8(1, h[22]);   // mono
  TEST_ASSERT_EQUAL_UINT8(16, h[34]);  // bits
  TEST_ASSERT_EQUAL_UINT8(0x40, h[24]);  // 8000 = 0x1F40 little endian
  TEST_ASSERT_EQUAL_UINT8(0x1F, h[25]);
  TEST_ASSERT_EQUAL_UINT8(0xE8, h[40]);  // 1000 = 0x03E8
  TEST_ASSERT_EQUAL_UINT8(0x03, h[41]);
  TEST_ASSERT_EQUAL_UINT8(0x0C, h[4]);  // RIFF size 1036 = 0x040C
  TEST_ASSERT_EQUAL_UINT8(0x04, h[5]);
}

void test_a_header_the_firmware_wrote_is_valid_for_its_file_size() {
  std::uint8_t h[kWavHeaderBytes];
  header(h, 8000, 16000);
  TEST_ASSERT_TRUE(valid_wav_header(h, kWavHeaderBytes + 16000));
  header(h, 16000, 32000 * 30);
  TEST_ASSERT_TRUE(valid_wav_header(h, kWavHeaderBytes + 32000 * 30));
}

void test_a_header_at_the_ten_minute_limit_is_valid_and_one_sample_over_is_not() {
  std::uint8_t h[kWavHeaderBytes];
  header(h, 16000, kMaxPcm);
  TEST_ASSERT_TRUE(valid_wav_header(h, kWavHeaderBytes + kMaxPcm));
  header(h, 16000, kMaxPcm + 2);
  TEST_ASSERT_FALSE(valid_wav_header(h, kWavHeaderBytes + kMaxPcm + 2));
}

void test_a_recording_from_the_earlier_8_khz_firmware_is_still_valid() {
  std::uint8_t h[kWavHeaderBytes];
  header(h, 8000, 1920000);  // the old two-minute format
  TEST_ASSERT_TRUE(valid_wav_header(h, kWavHeaderBytes + 1920000));
  header(h, 8000, 8000 * 2 * kMaxSeconds + 2);
  TEST_ASSERT_FALSE(valid_wav_header(h, kWavHeaderBytes + 8000 * 2 * kMaxSeconds + 2));
}

void test_rejects_a_file_whose_size_disagrees_with_its_header() {
  std::uint8_t h[kWavHeaderBytes];
  header(h, 8000, 16000);
  TEST_ASSERT_FALSE(valid_wav_header(h, kWavHeaderBytes + 15999));
  TEST_ASSERT_FALSE(valid_wav_header(h, kWavHeaderBytes + 16002));
  TEST_ASSERT_FALSE(valid_wav_header(h, kWavHeaderBytes - 1));
}

void test_rejects_unknown_rate_empty_or_odd_audio_and_wrong_format() {
  std::uint8_t h[kWavHeaderBytes];
  header(h, 44100, 1000);
  TEST_ASSERT_FALSE(valid_wav_header(h, kWavHeaderBytes + 1000));
  header(h, 8000, 0);
  TEST_ASSERT_FALSE(valid_wav_header(h, kWavHeaderBytes));
  header(h, 8000, 1001);
  TEST_ASSERT_FALSE(valid_wav_header(h, kWavHeaderBytes + 1001));
  header(h, 8000, 1000);
  h[22] = 2;  // stereo
  TEST_ASSERT_FALSE(valid_wav_header(h, kWavHeaderBytes + 1000));
  header(h, 8000, 1000);
  h[0] = 'X';
  TEST_ASSERT_FALSE(valid_wav_header(h, kWavHeaderBytes + 1000));
}

// --- samples ---------------------------------------------------------------------

void test_sample_conversion_keeps_the_top_16_bits_with_the_sign() {
  TEST_ASSERT_EQUAL_INT16(0, sample_to_pcm16(0));
  TEST_ASSERT_EQUAL_INT16(1, sample_to_pcm16(0x00010000));
  TEST_ASSERT_EQUAL_INT16(-1, sample_to_pcm16(-0x00010000));
  TEST_ASSERT_EQUAL_INT16(32767, sample_to_pcm16(0x7FFFFFFF));
  TEST_ASSERT_EQUAL_INT16(-32768, sample_to_pcm16(INT32_MIN));
}

// --- bytes_to_write ----------------------------------------------------------------

void test_a_chunk_that_fits_is_written_whole() {
  TEST_ASSERT_EQUAL_UINT32(512, bytes_to_write(0, 1000, 512));
}

void test_the_last_chunk_is_cut_at_the_limit_instead_of_overshooting() {
  TEST_ASSERT_EQUAL_UINT32(200, bytes_to_write(800, 1000, 512));
  TEST_ASSERT_EQUAL_UINT32(2, bytes_to_write(998, 1000, 512));
}

void test_nothing_is_written_at_or_past_the_limit() {
  TEST_ASSERT_EQUAL_UINT32(0, bytes_to_write(1000, 1000, 512));
  TEST_ASSERT_EQUAL_UINT32(0, bytes_to_write(1002, 1000, 512));
}

void test_a_cut_never_splits_a_16_bit_sample() {
  TEST_ASSERT_EQUAL_UINT32(2, bytes_to_write(997, 1000, 512));  // 3 bytes left -> 1 sample
  TEST_ASSERT_EQUAL_UINT32(510, bytes_to_write(0, 1000, 511));  // odd offer
}

// --- pause timeout -----------------------------------------------------------------

void test_pause_expires_exactly_at_the_limit() {
  TEST_ASSERT_FALSE(pause_expired(1000, 1000 + kMaxPauseMs - 1));
  TEST_ASSERT_TRUE(pause_expired(1000, 1000 + kMaxPauseMs));
}

void test_pause_expiry_survives_a_millis_wrap() {
  const std::uint32_t paused_at = 0xFFFFFF00u;
  TEST_ASSERT_FALSE(pause_expired(paused_at, 5000));
  TEST_ASSERT_TRUE(pause_expired(paused_at, paused_at + kMaxPauseMs));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_the_length_is_the_whole_seconds_that_fit_after_the_reserve);
  RUN_TEST(test_a_partial_second_is_not_promised);
  RUN_TEST(test_the_recording_plus_the_reserve_never_exceeds_the_free_space);
  RUN_TEST(test_the_minimum_is_five_seconds);
  RUN_TEST(test_the_length_is_capped_at_ten_minutes);
  RUN_TEST(test_no_free_space_fits_nothing);
  RUN_TEST(test_header_is_44_bytes_of_mono_16_bit_pcm);
  RUN_TEST(test_a_header_the_firmware_wrote_is_valid_for_its_file_size);
  RUN_TEST(test_a_header_at_the_ten_minute_limit_is_valid_and_one_sample_over_is_not);
  RUN_TEST(test_a_recording_from_the_earlier_8_khz_firmware_is_still_valid);
  RUN_TEST(test_rejects_a_file_whose_size_disagrees_with_its_header);
  RUN_TEST(test_rejects_unknown_rate_empty_or_odd_audio_and_wrong_format);
  RUN_TEST(test_sample_conversion_keeps_the_top_16_bits_with_the_sign);
  RUN_TEST(test_a_chunk_that_fits_is_written_whole);
  RUN_TEST(test_the_last_chunk_is_cut_at_the_limit_instead_of_overshooting);
  RUN_TEST(test_nothing_is_written_at_or_past_the_limit);
  RUN_TEST(test_a_cut_never_splits_a_16_bit_sample);
  RUN_TEST(test_pause_expires_exactly_at_the_limit);
  RUN_TEST(test_pause_expiry_survives_a_millis_wrap);
  return UNITY_END();
}
