#include <unity.h>

#include <cstddef>
#include <cstring>
#include <string>

#include "rsvp.h"

void setUp() {}
void tearDown() {}

namespace {

// Returns the word at `offset` as a std::string, or "" when none is found.
std::string word_at(const char* text, std::size_t offset) {
  auto span = leeloo::rsvp::next_word(text, std::strlen(text), offset);
  if (!span) return "";
  return std::string(text + span->start, span->length);
}

}  // namespace

// --- next_word ---------------------------------------------------------------

void test_next_word_returns_first_word_from_start() {
  auto span = leeloo::rsvp::next_word("hola mundo", 10, 0);
  TEST_ASSERT_TRUE(span.has_value());
  TEST_ASSERT_EQUAL_UINT(0, span->start);
  TEST_ASSERT_EQUAL_UINT(4, span->length);
}

void test_next_word_skips_leading_spaces() {
  TEST_ASSERT_EQUAL_STRING("hola", word_at("   hola", 0).c_str());
}

void test_next_word_treats_tabs_and_newlines_as_separators() {
  TEST_ASSERT_EQUAL_STRING("b", word_at("a\tb\nc", 1).c_str());
}

void test_next_word_returns_nothing_when_only_spaces_remain() {
  auto span = leeloo::rsvp::next_word("hola   ", 7, 4);
  TEST_ASSERT_FALSE(span.has_value());
}

void test_next_word_returns_nothing_for_empty_input() {
  TEST_ASSERT_FALSE(leeloo::rsvp::next_word("", 0, 0).has_value());
}

void test_next_word_returns_nothing_past_end() {
  TEST_ASSERT_FALSE(leeloo::rsvp::next_word("hola", 4, 5).has_value());
}

void test_next_word_keeps_spanish_word_with_accents_whole() {
  // "ñandú" is 7 bytes: ñ(2) a n d ú(2).
  const char* phrase = "ñandú ¡sí!";
  auto span = leeloo::rsvp::next_word(phrase, std::strlen(phrase), 0);
  TEST_ASSERT_TRUE(span.has_value());
  TEST_ASSERT_EQUAL_UINT(0, span->start);
  TEST_ASSERT_EQUAL_UINT(7, span->length);
}

void test_next_word_rejects_offset_inside_utf8_codepoint() {
  // Byte 1 of "ñu" is the continuation byte of ñ.
  TEST_ASSERT_FALSE(leeloo::rsvp::next_word("ñu", 3, 1).has_value());
}

void test_next_word_rejects_null_text_with_nonzero_size() {
  TEST_ASSERT_FALSE(leeloo::rsvp::next_word(nullptr, 4, 0).has_value());
}

void test_next_word_iterates_phrase_without_splitting_codepoints() {
  const char* phrase = "¿Qué pasó? ¡Sí!";
  std::size_t size = std::strlen(phrase);
  std::size_t offset = 0;
  int count = 0;
  while (auto span = leeloo::rsvp::next_word(phrase, size, offset)) {
    ++count;
    offset = span->start + span->length;
  }
  TEST_ASSERT_EQUAL_INT(3, count);
}

void test_next_word_handles_long_input() {
  std::string big(4096, 'a');
  auto span = leeloo::rsvp::next_word(big.data(), big.size(), 0);
  TEST_ASSERT_TRUE(span.has_value());
  TEST_ASSERT_EQUAL_UINT(4096, span->length);
}

// --- focal_split (ORP) -------------------------------------------------------

void test_focal_split_uses_first_letter_for_one_letter_word() {
  auto s = leeloo::rsvp::focal_split("a", 1);
  TEST_ASSERT_EQUAL_UINT(0, s.prefix_bytes);
  TEST_ASSERT_EQUAL_UINT(1, s.focal_bytes);
}

void test_focal_split_uses_second_letter_for_short_words() {
  auto s = leeloo::rsvp::focal_split("hola", 4);
  TEST_ASSERT_EQUAL_UINT(1, s.prefix_bytes);
  TEST_ASSERT_EQUAL_UINT(1, s.focal_bytes);
}

void test_focal_split_counts_codepoints_not_bytes() {
  // "ñandú": ORP index 1 is 'a', which sits after the 2-byte ñ.
  auto s = leeloo::rsvp::focal_split("ñandú", 7);
  TEST_ASSERT_EQUAL_UINT(2, s.prefix_bytes);
  TEST_ASSERT_EQUAL_UINT(1, s.focal_bytes);
}

void test_focal_split_handles_multibyte_focal_letter() {
  // "uñ": ORP index 1 is 'ñ', a 2-byte code point.
  auto s = leeloo::rsvp::focal_split("uñ", 3);
  TEST_ASSERT_EQUAL_UINT(1, s.prefix_bytes);
  TEST_ASSERT_EQUAL_UINT(2, s.focal_bytes);
}

void test_focal_split_grows_with_word_length() {
  TEST_ASSERT_EQUAL_UINT(2, leeloo::rsvp::focal_split("abcdef", 6).prefix_bytes);
  TEST_ASSERT_EQUAL_UINT(3, leeloo::rsvp::focal_split("abcdefghij", 10).prefix_bytes);
  TEST_ASSERT_EQUAL_UINT(4, leeloo::rsvp::focal_split("abcdefghijklmn", 14).prefix_bytes);
}

void test_focal_split_caps_very_long_words_at_index_four() {
  std::string big(500, 'x');
  auto s = leeloo::rsvp::focal_split(big.data(), big.size());
  TEST_ASSERT_EQUAL_UINT(4, s.prefix_bytes);
  TEST_ASSERT_EQUAL_UINT(1, s.focal_bytes);
}

void test_focal_split_returns_zero_for_empty_word() {
  auto s = leeloo::rsvp::focal_split("", 0);
  TEST_ASSERT_EQUAL_UINT(0, s.prefix_bytes);
  TEST_ASSERT_EQUAL_UINT(0, s.focal_bytes);
}

void test_focal_split_caps_focal_bytes_at_utf8_max_for_malformed_input() {
  // Lead byte of a 2-byte sequence followed by many continuation bytes.
  std::string malformed = "\xC3";
  malformed.append(40, '\x80');
  auto s = leeloo::rsvp::focal_split(malformed.data(), malformed.size());
  TEST_ASSERT_EQUAL_UINT(0, s.prefix_bytes);
  TEST_ASSERT_EQUAL_UINT(4, s.focal_bytes);
}

void test_focal_split_switches_index_at_five_six_nine_ten_thirteen_fourteen() {
  TEST_ASSERT_EQUAL_UINT(1, leeloo::rsvp::focal_split("abcde", 5).prefix_bytes);
  TEST_ASSERT_EQUAL_UINT(2, leeloo::rsvp::focal_split("abcdef", 6).prefix_bytes);
  TEST_ASSERT_EQUAL_UINT(2, leeloo::rsvp::focal_split("abcdefghi", 9).prefix_bytes);
  TEST_ASSERT_EQUAL_UINT(3, leeloo::rsvp::focal_split("abcdefghij", 10).prefix_bytes);
  TEST_ASSERT_EQUAL_UINT(3, leeloo::rsvp::focal_split("abcdefghijklm", 13).prefix_bytes);
  TEST_ASSERT_EQUAL_UINT(4, leeloo::rsvp::focal_split("abcdefghijklmn", 14).prefix_bytes);
}

void test_focal_split_returns_whole_word_as_focal_for_only_continuation_bytes() {
  std::string stray(3, '\x80');
  auto s = leeloo::rsvp::focal_split(stray.data(), stray.size());
  TEST_ASSERT_EQUAL_UINT(stray.size(), s.prefix_bytes);
  TEST_ASSERT_EQUAL_UINT(0, s.focal_bytes);
}

void test_next_word_returns_nothing_at_exact_end() {
  TEST_ASSERT_FALSE(leeloo::rsvp::next_word("hola", 4, 4).has_value());
}

// --- split_point (syllable break for long words) -----------------------------

void test_split_point_splits_casa_between_ca_and_sa() {
  TEST_ASSERT_EQUAL_UINT(2, leeloo::rsvp::split_point("casa", 4));
}

void test_split_point_splits_extraordinariamente_after_extraordi() {
  const char* word = "Extraordinariamente";
  TEST_ASSERT_EQUAL_UINT(9, leeloo::rsvp::split_point(word, std::strlen(word)));
}

void test_split_point_splits_construccion_at_syllable_boundary() {
  // Candidates at byte 4 ("cons|truc...") and byte 8 are equally close to the
  // middle; the earlier one wins.
  const char* word = "construcción";
  TEST_ASSERT_EQUAL_UINT(4, leeloo::rsvp::split_point(word, std::strlen(word)));
}

void test_split_point_keeps_digraphs_ch_ll_rr_together() {
  // "pollo" is po|llo, never p|ollo or pol|lo.
  TEST_ASSERT_EQUAL_UINT(2, leeloo::rsvp::split_point("pollo", 5));
  // "muchacho": the boundary before "ch" is the one picked, never inside it.
  const char* word = "muchacho";
  const std::size_t split = leeloo::rsvp::split_point(word, std::strlen(word));
  TEST_ASSERT_EQUAL_UINT(5, split);  // mucha|cho
}

void test_split_point_keeps_diphthongs_together() {
  // "cuento" is cuen|to: "ue" is a diphthong and must not be cut.
  TEST_ASSERT_EQUAL_UINT(4, leeloo::rsvp::split_point("cuento", 6));
}

void test_split_point_splits_hiatus_between_strong_vowels() {
  TEST_ASSERT_EQUAL_UINT(2, leeloo::rsvp::split_point("leer", 4));  // le|er
  TEST_ASSERT_EQUAL_UINT(2, leeloo::rsvp::split_point("caos", 4)); // ca|os
}

void test_split_point_splits_on_utf8_codepoint_boundary_with_enie_and_accents() {
  // "ñandú": byte 4 is 'd', right after the 2-byte ñ and the "ñan" part.
  TEST_ASSERT_EQUAL_UINT(4, leeloo::rsvp::split_point("ñandú", 7));
}

void test_split_point_ignores_leading_and_trailing_punctuation() {
  // "¿Cómo?" is 8 bytes; the boundary is before 'm' at byte 5 (cóm|o).
  const char* word = "¿Cómo?";
  TEST_ASSERT_EQUAL_UINT(5, leeloo::rsvp::split_point(word, std::strlen(word)));
}

void test_split_point_returns_zero_for_one_syllable_word() {
  // "pan" has no syllable break; splitting it would cut inside the syllable.
  TEST_ASSERT_EQUAL_UINT(0, leeloo::rsvp::split_point("pan", 3));
}

void test_split_point_returns_zero_for_fewer_than_two_letters() {
  TEST_ASSERT_EQUAL_UINT(0, leeloo::rsvp::split_point("a", 1));
  TEST_ASSERT_EQUAL_UINT(0, leeloo::rsvp::split_point("", 0));
}

void test_split_point_handles_long_input() {
  std::string big;
  for (int i = 0; i < 2048; ++i) big += "ma";  // 4096 letters, many candidates
  const std::size_t split = leeloo::rsvp::split_point(big.data(), big.size());
  TEST_ASSERT_TRUE(split > 0 && split < big.size());
}

// --- hold_percent (pauses after punctuation, paragraphs, long words) ---------

namespace {

// Hold for the first word of `text`.
std::uint32_t hold_of(const char* text) {
  const std::size_t size = std::strlen(text);
  auto span = leeloo::rsvp::next_word(text, size, 0);
  if (!span) return 0;
  return leeloo::rsvp::hold_percent(text, size, *span);
}

}  // namespace

void test_hold_is_normal_for_plain_word() {
  TEST_ASSERT_EQUAL_UINT32(100, hold_of("hola"));
}

void test_hold_is_comma_level_for_comma_semicolon_colon() {
  TEST_ASSERT_EQUAL_UINT32(200, hold_of("hola,"));
  TEST_ASSERT_EQUAL_UINT32(200, hold_of("hola;"));
  TEST_ASSERT_EQUAL_UINT32(200, hold_of("hola:"));
}

void test_hold_is_sentence_level_for_period_question_exclamation() {
  TEST_ASSERT_EQUAL_UINT32(300, hold_of("hola."));
  TEST_ASSERT_EQUAL_UINT32(300, hold_of("hola?"));
  TEST_ASSERT_EQUAL_UINT32(300, hold_of("hola!"));
}

void test_hold_is_sentence_level_for_ellipsis_character_and_three_dots() {
  TEST_ASSERT_EQUAL_UINT32(300, hold_of("pues…"));
  TEST_ASSERT_EQUAL_UINT32(300, hold_of("pues..."));
}

void test_hold_is_comma_level_for_closing_quote_paren_and_dash() {
  TEST_ASSERT_EQUAL_UINT32(200, hold_of("hola\""));
  TEST_ASSERT_EQUAL_UINT32(200, hold_of("(hola)"));
  TEST_ASSERT_EQUAL_UINT32(200, hold_of("hola—"));
}

void test_hold_takes_largest_of_stacked_trailing_marks() {
  TEST_ASSERT_EQUAL_UINT32(300, hold_of("hola.\""));     // ." -> sentence
  TEST_ASSERT_EQUAL_UINT32(300, hold_of("¿qué?»"));  // ?» -> sentence
  TEST_ASSERT_EQUAL_UINT32(300, hold_of("(hola.)"));     // .) -> sentence
}

void test_hold_ignores_opening_marks_at_word_start() {
  TEST_ASSERT_EQUAL_UINT32(100, hold_of("¿Por"));    // ¿Por
  TEST_ASSERT_EQUAL_UINT32(100, hold_of("¡Sí"));  // ¡Sí
  TEST_ASSERT_EQUAL_UINT32(100, hold_of("(hola"));
  TEST_ASSERT_EQUAL_UINT32(100, hold_of("—Hola"));    // —Hola
}

void test_hold_is_paragraph_level_after_blank_line() {
  TEST_ASSERT_EQUAL_UINT32(500, hold_of("fallo.\n\nLos"));
}

void test_hold_ignores_carriage_returns_in_paragraph_gap() {
  TEST_ASSERT_EQUAL_UINT32(500, hold_of("hola.\r\n\r\nAdiós"));
}

void test_hold_treats_single_newline_as_plain_space() {
  // One newline is a wrap, not a paragraph: the period still gives sentence level.
  TEST_ASSERT_EQUAL_UINT32(300, hold_of("hola.\nadios"));
  TEST_ASSERT_EQUAL_UINT32(100, hold_of("hola\nadios"));
}

void test_hold_is_paragraph_level_at_end_of_text_with_trailing_blank_line() {
  TEST_ASSERT_EQUAL_UINT32(500, hold_of("hola\n\n"));
}

void test_hold_adds_bonus_for_long_words_counting_codepoints_not_bytes() {
  TEST_ASSERT_EQUAL_UINT32(100, hold_of("añoranza"));    // 8 letters
  TEST_ASSERT_EQUAL_UINT32(125, hold_of("añoranzas"));   // 9 letters
}

void test_hold_punctuation_wins_over_long_word_bonus() {
  TEST_ASSERT_EQUAL_UINT32(300, hold_of("añoranzas."));  // 300 > 125
}

void test_hold_handles_punctuation_only_token() {
  TEST_ASSERT_EQUAL_UINT32(300, hold_of("..."));
  TEST_ASSERT_EQUAL_UINT32(200, hold_of("—"));
}

void test_hold_handles_digits_and_decimals() {
  TEST_ASSERT_EQUAL_UINT32(100, hold_of("3,5"));
  TEST_ASSERT_EQUAL_UINT32(200, hold_of("3,5,"));
}

void test_hold_handles_empty_and_null_input() {
  TEST_ASSERT_EQUAL_UINT32(100, leeloo::rsvp::hold_percent(nullptr, 0, {0, 0}));
  TEST_ASSERT_EQUAL_UINT32(100, leeloo::rsvp::hold_percent("", 0, {0, 0}));
}

void test_hold_handles_long_input() {
  std::string big(4096, 'a');
  big += '.';
  auto span = leeloo::rsvp::next_word(big.data(), big.size(), 0);
  TEST_ASSERT_TRUE(span.has_value());
  TEST_ASSERT_EQUAL_UINT32(300, leeloo::rsvp::hold_percent(big.data(), big.size(), *span));
}

// --- word_delay_ms -----------------------------------------------------------

void test_word_delay_is_200ms_at_300_wpm() {
  TEST_ASSERT_EQUAL_UINT32(200, leeloo::rsvp::word_delay_ms(300));
}

void test_word_delay_clamps_wpm_to_bounds() {
  TEST_ASSERT_EQUAL_UINT32(400, leeloo::rsvp::word_delay_ms(0));       // 150 WPM
  TEST_ASSERT_EQUAL_UINT32(400, leeloo::rsvp::word_delay_ms(150));
  TEST_ASSERT_EQUAL_UINT32(75, leeloo::rsvp::word_delay_ms(800));
  TEST_ASSERT_EQUAL_UINT32(75, leeloo::rsvp::word_delay_ms(10000));   // 800 WPM
}

void test_word_delay_snaps_wpm_to_25_steps() {
  TEST_ASSERT_EQUAL_UINT32(200, leeloo::rsvp::word_delay_ms(310));  // -> 300
  TEST_ASSERT_EQUAL_UINT32(184, leeloo::rsvp::word_delay_ms(313));  // -> 325
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_next_word_returns_first_word_from_start);
  RUN_TEST(test_next_word_skips_leading_spaces);
  RUN_TEST(test_next_word_treats_tabs_and_newlines_as_separators);
  RUN_TEST(test_next_word_returns_nothing_when_only_spaces_remain);
  RUN_TEST(test_next_word_returns_nothing_for_empty_input);
  RUN_TEST(test_next_word_returns_nothing_past_end);
  RUN_TEST(test_next_word_keeps_spanish_word_with_accents_whole);
  RUN_TEST(test_next_word_rejects_offset_inside_utf8_codepoint);
  RUN_TEST(test_next_word_rejects_null_text_with_nonzero_size);
  RUN_TEST(test_next_word_iterates_phrase_without_splitting_codepoints);
  RUN_TEST(test_next_word_handles_long_input);
  RUN_TEST(test_focal_split_uses_first_letter_for_one_letter_word);
  RUN_TEST(test_focal_split_uses_second_letter_for_short_words);
  RUN_TEST(test_focal_split_counts_codepoints_not_bytes);
  RUN_TEST(test_focal_split_handles_multibyte_focal_letter);
  RUN_TEST(test_focal_split_grows_with_word_length);
  RUN_TEST(test_focal_split_caps_very_long_words_at_index_four);
  RUN_TEST(test_focal_split_returns_zero_for_empty_word);
  RUN_TEST(test_focal_split_caps_focal_bytes_at_utf8_max_for_malformed_input);
  RUN_TEST(test_focal_split_switches_index_at_five_six_nine_ten_thirteen_fourteen);
  RUN_TEST(test_focal_split_returns_whole_word_as_focal_for_only_continuation_bytes);
  RUN_TEST(test_next_word_returns_nothing_at_exact_end);
  RUN_TEST(test_split_point_splits_casa_between_ca_and_sa);
  RUN_TEST(test_split_point_splits_extraordinariamente_after_extraordi);
  RUN_TEST(test_split_point_splits_construccion_at_syllable_boundary);
  RUN_TEST(test_split_point_keeps_digraphs_ch_ll_rr_together);
  RUN_TEST(test_split_point_keeps_diphthongs_together);
  RUN_TEST(test_split_point_splits_hiatus_between_strong_vowels);
  RUN_TEST(test_split_point_splits_on_utf8_codepoint_boundary_with_enie_and_accents);
  RUN_TEST(test_split_point_ignores_leading_and_trailing_punctuation);
  RUN_TEST(test_split_point_returns_zero_for_one_syllable_word);
  RUN_TEST(test_split_point_returns_zero_for_fewer_than_two_letters);
  RUN_TEST(test_split_point_handles_long_input);
  RUN_TEST(test_hold_is_normal_for_plain_word);
  RUN_TEST(test_hold_is_comma_level_for_comma_semicolon_colon);
  RUN_TEST(test_hold_is_sentence_level_for_period_question_exclamation);
  RUN_TEST(test_hold_is_sentence_level_for_ellipsis_character_and_three_dots);
  RUN_TEST(test_hold_is_comma_level_for_closing_quote_paren_and_dash);
  RUN_TEST(test_hold_takes_largest_of_stacked_trailing_marks);
  RUN_TEST(test_hold_ignores_opening_marks_at_word_start);
  RUN_TEST(test_hold_is_paragraph_level_after_blank_line);
  RUN_TEST(test_hold_ignores_carriage_returns_in_paragraph_gap);
  RUN_TEST(test_hold_treats_single_newline_as_plain_space);
  RUN_TEST(test_hold_is_paragraph_level_at_end_of_text_with_trailing_blank_line);
  RUN_TEST(test_hold_adds_bonus_for_long_words_counting_codepoints_not_bytes);
  RUN_TEST(test_hold_punctuation_wins_over_long_word_bonus);
  RUN_TEST(test_hold_handles_punctuation_only_token);
  RUN_TEST(test_hold_handles_digits_and_decimals);
  RUN_TEST(test_hold_handles_empty_and_null_input);
  RUN_TEST(test_hold_handles_long_input);
  RUN_TEST(test_word_delay_is_200ms_at_300_wpm);
  RUN_TEST(test_word_delay_clamps_wpm_to_bounds);
  RUN_TEST(test_word_delay_snaps_wpm_to_25_steps);
  return UNITY_END();
}
