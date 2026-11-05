#include <unity.h>

#include <cstring>

#include "text.h"

using namespace leeloo::text;

void setUp() {}
void tearDown() {}

namespace {

std::size_t truncate(const char* s, std::size_t max_chars, char* out, std::size_t cap) {
  return truncate_chars(s, std::strlen(s), max_chars, "...", out, cap);
}

}  // namespace

void test_empty_text_stays_empty() {
  char out[16];
  TEST_ASSERT_EQUAL_UINT(0, truncate("", 5, out, sizeof out));
  TEST_ASSERT_EQUAL_STRING("", out);
}

void test_text_that_fits_is_copied_unchanged() {
  char out[32];
  TEST_ASSERT_EQUAL_UINT(4, truncate("Leer", 4, out, sizeof out));
  TEST_ASSERT_EQUAL_STRING("Leer", out);
}

void test_long_text_is_cut_and_gets_an_ellipsis() {
  char out[32];
  truncate("Don Quijote de la Mancha", 10, out, sizeof out);
  TEST_ASSERT_EQUAL_STRING("Don Qui...", out);  // 7 kept characters + 3 for "..."
}

void test_counts_spanish_letters_as_one_character() {
  char out[32];
  // 7 characters, 10 bytes: fits in 7, so no cut even though bytes > characters.
  TEST_ASSERT_EQUAL_UINT(10, truncate("¡Ñandú!", 7, out, sizeof out));
  TEST_ASSERT_EQUAL_STRING("¡Ñandú!", out);
}

void test_never_splits_a_multibyte_character() {
  char out[32];
  truncate("ñññññññ", 5, out, sizeof out);  // keep 2 letters + "..."
  TEST_ASSERT_EQUAL_STRING("ññ...", out);
}

void test_max_smaller_than_ellipsis_returns_a_prefix_of_the_ellipsis() {
  char out[32];
  truncate("abcdef", 2, out, sizeof out);
  TEST_ASSERT_EQUAL_STRING("..", out);
}

void test_output_buffer_too_small_returns_zero() {
  char out[4];
  TEST_ASSERT_EQUAL_UINT(0, truncate("abcdefgh", 6, out, sizeof out));
}

void test_invalid_utf8_input_is_rejected() {
  char out[16];
  TEST_ASSERT_EQUAL_UINT(0, truncate_chars("a\xC3", 2, 5, "...", out, sizeof out));
}

// --- scroll window ---

void test_window_starts_at_top_for_short_list() {
  TEST_ASSERT_EQUAL_UINT(0, scroll_start(0, 2, 3, 4));
}

void test_window_moves_down_to_keep_cursor_visible() {
  TEST_ASSERT_EQUAL_UINT(1, scroll_start(0, 4, 5, 4));  // cursor 4 needs rows 1..4
}

void test_window_moves_up_when_cursor_goes_above_it() {
  TEST_ASSERT_EQUAL_UINT(2, scroll_start(3, 2, 6, 4));
}

void test_wrap_from_last_to_first_jumps_to_top() {
  TEST_ASSERT_EQUAL_UINT(0, scroll_start(2, 0, 6, 4));
}

void test_wrap_from_first_to_last_jumps_to_bottom() {
  TEST_ASSERT_EQUAL_UINT(2, scroll_start(0, 5, 6, 4));
}

void test_window_never_scrolls_past_the_end() {
  TEST_ASSERT_EQUAL_UINT(2, scroll_start(2, 5, 6, 4));
}

void test_window_keeps_cursor_visible_on_empty_list() {
  TEST_ASSERT_EQUAL_UINT(0, scroll_start(0, 0, 0, 4));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_empty_text_stays_empty);
  RUN_TEST(test_text_that_fits_is_copied_unchanged);
  RUN_TEST(test_long_text_is_cut_and_gets_an_ellipsis);
  RUN_TEST(test_counts_spanish_letters_as_one_character);
  RUN_TEST(test_never_splits_a_multibyte_character);
  RUN_TEST(test_max_smaller_than_ellipsis_returns_a_prefix_of_the_ellipsis);
  RUN_TEST(test_output_buffer_too_small_returns_zero);
  RUN_TEST(test_invalid_utf8_input_is_rejected);
  RUN_TEST(test_window_starts_at_top_for_short_list);
  RUN_TEST(test_window_moves_down_to_keep_cursor_visible);
  RUN_TEST(test_window_moves_up_when_cursor_goes_above_it);
  RUN_TEST(test_wrap_from_last_to_first_jumps_to_top);
  RUN_TEST(test_wrap_from_first_to_last_jumps_to_bottom);
  RUN_TEST(test_window_never_scrolls_past_the_end);
  RUN_TEST(test_window_keeps_cursor_visible_on_empty_list);
  return UNITY_END();
}
