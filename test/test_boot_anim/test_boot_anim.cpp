#include <unity.h>

#include "boot_anim.h"

using namespace leeloo::boot_anim;

void setUp() {}
void tearDown() {}

// --- the hand ----------------------------------------------------------------------

void test_the_hand_starts_stacked_on_the_top_guide() {
  TEST_ASSERT_EQUAL_INT(0, frame_at(0).hand_degrees);
}

void test_the_hand_ends_pointing_down_and_stays_there() {
  TEST_ASSERT_EQUAL_INT(180, frame_at(kSweepMs).hand_degrees);
  TEST_ASSERT_EQUAL_INT(180, frame_at(kTotalMs).hand_degrees);
  TEST_ASSERT_EQUAL_INT(180, frame_at(kTotalMs + 100000).hand_degrees);
}

void test_the_hand_never_moves_backwards() {
  int previous = 0;
  for (std::uint32_t t = 0; t <= kTotalMs; t += 7) {
    const int now = frame_at(t).hand_degrees;
    TEST_ASSERT_TRUE(now >= previous);
    previous = now;
  }
}

void test_the_hand_eases_out_fast_at_first_then_slowly() {
  const int first_quarter = frame_at(kSweepMs / 4).hand_degrees;
  const int last_quarter = frame_at(kSweepMs).hand_degrees - frame_at(kSweepMs * 3 / 4).hand_degrees;
  TEST_ASSERT_TRUE(first_quarter > 90);  // most of the turn happens early
  TEST_ASSERT_TRUE(last_quarter < 20);   // and the end is gentle
}

void test_the_hand_is_between_the_ends_during_the_sweep() {
  const int mid = frame_at(kSweepMs / 2).hand_degrees;
  TEST_ASSERT_TRUE(mid > 0 && mid < 180);
}

// --- the word ----------------------------------------------------------------------

void test_no_letters_at_first_then_one_every_110_ms() {
  TEST_ASSERT_EQUAL_INT(0, frame_at(0).letters);
  TEST_ASSERT_EQUAL_INT(0, frame_at(kLetterMs - 1).letters);
  TEST_ASSERT_EQUAL_INT(1, frame_at(kLetterMs).letters);
  TEST_ASSERT_EQUAL_INT(3, frame_at(3 * kLetterMs + 5).letters);
}

void test_all_six_letters_of_leeloo_show_after_660_ms_and_no_more() {
  TEST_ASSERT_EQUAL_INT(kWordLetters, frame_at(6 * kLetterMs).letters);
  TEST_ASSERT_EQUAL_INT(kWordLetters, frame_at(kTotalMs).letters);
  TEST_ASSERT_EQUAL_INT(6, kWordLetters);
  TEST_ASSERT_EQUAL_STRING("leeloo", kWord);
}

void test_the_word_is_complete_before_the_beat_starts() {
  TEST_ASSERT_TRUE(6 * kLetterMs <= kBeatStartMs);
}

// --- the beat ----------------------------------------------------------------------

void test_the_dot_is_nominal_outside_the_beat() {
  TEST_ASSERT_EQUAL_INT(1000, frame_at(0).dot_permille);
  TEST_ASSERT_EQUAL_INT(1000, frame_at(kBeatStartMs - 1).dot_permille);
  TEST_ASSERT_EQUAL_INT(1000, frame_at(kBeatStartMs).dot_permille);
  TEST_ASSERT_EQUAL_INT(1000, frame_at(kBeatStartMs + kBeatMs).dot_permille);
  TEST_ASSERT_EQUAL_INT(1000, frame_at(kTotalMs).dot_permille);
}

void test_the_beat_follows_the_loudop_keyframes() {
  // 28 % 1.13, 55 % 0.98, 78 % 1.07 of the 340 ms beat.
  // The keyframes fall between whole milliseconds (28 % of 340 ms is 95.2 ms), so a few
  // thousandths of tolerance.
  TEST_ASSERT_INT_WITHIN(5, 1130, frame_at(kBeatStartMs + kBeatMs * 28 / 100).dot_permille);
  TEST_ASSERT_INT_WITHIN(5, 980, frame_at(kBeatStartMs + kBeatMs * 55 / 100).dot_permille);
  TEST_ASSERT_INT_WITHIN(5, 1070, frame_at(kBeatStartMs + kBeatMs * 78 / 100).dot_permille);
  TEST_ASSERT_TRUE(frame_at(kBeatStartMs + kBeatMs * 14 / 100).dot_permille > 1000);  // rising
  TEST_ASSERT_TRUE(frame_at(kBeatStartMs + kBeatMs * 42 / 100).dot_permille < 1130);  // falling
}

void test_the_beat_stays_inside_the_keyframe_range() {
  for (std::uint32_t t = kBeatStartMs; t <= kBeatStartMs + kBeatMs; ++t) {
    const int scale = frame_at(t).dot_permille;
    TEST_ASSERT_TRUE(scale >= 980 && scale <= 1130);  // the lowest and highest keyframes
  }
}

void test_the_beating_dot_never_reaches_the_guides() {
  for (std::uint32_t t = 0; t <= kTotalMs; ++t) {
    const int radius = (kDotRadius * frame_at(t).dot_permille + 500) / 1000;  // as the drawing rounds it
    TEST_ASSERT_TRUE(radius < kHandInner);
  }
}

void test_the_last_millisecond_of_the_sweep_is_almost_all_the_way_round() {
  const int degrees = frame_at(kSweepMs - 1).hand_degrees;
  TEST_ASSERT_TRUE(degrees >= 179 && degrees <= 180);  // the last cell of the easing table
}

// --- the whole -----------------------------------------------------------------------

void test_it_lasts_about_two_seconds_and_reports_done_only_at_the_end() {
  TEST_ASSERT_TRUE(kTotalMs >= 1800 && kTotalMs <= 2200);
  TEST_ASSERT_FALSE(frame_at(0).done);
  TEST_ASSERT_FALSE(frame_at(kTotalMs - 1).done);
  TEST_ASSERT_TRUE(frame_at(kTotalMs).done);
  TEST_ASSERT_TRUE(frame_at(kTotalMs + 5000).done);
}

void test_the_beat_happens_after_the_sweep_and_before_the_end() {
  TEST_ASSERT_TRUE(kBeatStartMs >= kSweepMs);
  TEST_ASSERT_TRUE(kBeatStartMs + kBeatMs < kTotalMs);  // a pause at the end to read it
}

// --- geometry ------------------------------------------------------------------------

void test_the_hand_points_up_right_down_at_0_90_180_degrees() {
  const Offset up = polar(0, 20);
  TEST_ASSERT_EQUAL_INT(0, up.x);
  TEST_ASSERT_EQUAL_INT(-20, up.y);
  const Offset right = polar(90, 20);
  TEST_ASSERT_EQUAL_INT(20, right.x);
  TEST_ASSERT_EQUAL_INT(0, right.y);
  const Offset down = polar(180, 20);
  TEST_ASSERT_EQUAL_INT(0, down.x);
  TEST_ASSERT_EQUAL_INT(20, down.y);
}

void test_the_hand_turns_clockwise_through_the_right_side() {
  const Offset p = polar(45, 20);
  TEST_ASSERT_TRUE(p.x > 0);  // right of the dot, not left
  TEST_ASSERT_TRUE(p.y < 0);  // still in the upper half
  TEST_ASSERT_INT_WITHIN(1, 14, p.x);
  TEST_ASSERT_INT_WITHIN(1, -14, p.y);
}

void test_the_hand_at_the_top_covers_exactly_the_span_of_the_fixed_guide() {
  // The fixed guide is drawn from kHandOuter above the centre down to kHandInner: the hand at 0
  // degrees must start and end on those same rows, so the two read as one guide.
  TEST_ASSERT_EQUAL_INT(-kHandInner, polar(0, kHandInner).y);
  TEST_ASSERT_EQUAL_INT(-kHandOuter, polar(0, kHandOuter).y);
  TEST_ASSERT_EQUAL_INT(0, polar(0, kHandOuter).x);
  // And at 180 degrees it is the mirror image of it, below the dot.
  TEST_ASSERT_EQUAL_INT(kHandInner, polar(180, kHandInner).y);
  TEST_ASSERT_EQUAL_INT(kHandOuter, polar(180, kHandOuter).y);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_the_hand_starts_stacked_on_the_top_guide);
  RUN_TEST(test_the_hand_ends_pointing_down_and_stays_there);
  RUN_TEST(test_the_hand_never_moves_backwards);
  RUN_TEST(test_the_hand_eases_out_fast_at_first_then_slowly);
  RUN_TEST(test_the_hand_is_between_the_ends_during_the_sweep);
  RUN_TEST(test_no_letters_at_first_then_one_every_110_ms);
  RUN_TEST(test_all_six_letters_of_leeloo_show_after_660_ms_and_no_more);
  RUN_TEST(test_the_word_is_complete_before_the_beat_starts);
  RUN_TEST(test_the_dot_is_nominal_outside_the_beat);
  RUN_TEST(test_the_beat_follows_the_loudop_keyframes);
  RUN_TEST(test_the_beat_stays_inside_the_keyframe_range);
  RUN_TEST(test_the_beating_dot_never_reaches_the_guides);
  RUN_TEST(test_the_last_millisecond_of_the_sweep_is_almost_all_the_way_round);
  RUN_TEST(test_it_lasts_about_two_seconds_and_reports_done_only_at_the_end);
  RUN_TEST(test_the_beat_happens_after_the_sweep_and_before_the_end);
  RUN_TEST(test_the_hand_points_up_right_down_at_0_90_180_degrees);
  RUN_TEST(test_the_hand_turns_clockwise_through_the_right_side);
  RUN_TEST(test_the_hand_at_the_top_covers_exactly_the_span_of_the_fixed_guide);
  return UNITY_END();
}
