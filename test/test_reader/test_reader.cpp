#include <unity.h>

#include <cstddef>
#include <cstring>
#include <string>

#include "reader.h"

using leeloo::reader::body_offset;
using leeloo::reader::Reader;
using leeloo::reader::State;

void setUp() {}
void tearDown() {}

namespace {

// 300 WPM is 200 ms per word; the tests below depend on that default.
constexpr std::uint32_t kStartMs = 0;

// Text of the word the reader is showing, or "" when none.
std::string shown(const Reader& r) {
  const auto span = r.word();
  if (!span) return "";
  return std::string(r.body() + span->start, span->length);
}

}  // namespace

// --- body_offset -------------------------------------------------------------

void test_body_offset_points_after_start_marker_line() {
  const char* text = "TITLE: Libro\nAUTHOR: Ana\n###START###\nhola mundo";
  const auto offset = body_offset(text, std::strlen(text));
  TEST_ASSERT_TRUE(offset.has_value());
  TEST_ASSERT_EQUAL_STRING("hola mundo", text + *offset);
}

void test_body_offset_is_nullopt_without_start_marker() {
  const char* text = "TITLE: Libro\nAUTHOR: Ana\nhola mundo";
  TEST_ASSERT_FALSE(body_offset(text, std::strlen(text)).has_value());
}

void test_body_offset_is_nullopt_for_empty_input() {
  TEST_ASSERT_FALSE(body_offset("", 0).has_value());
}

// --- starting point ----------------------------------------------------------

void test_starts_on_first_word_of_body() {
  const char* body = "uno dos tres";
  Reader r(body, std::strlen(body), 0, kStartMs);
  TEST_ASSERT_EQUAL(State::Reading, r.state());
  TEST_ASSERT_EQUAL_STRING("uno", shown(r).c_str());
}

void test_empty_body_is_finished_immediately() {
  Reader r("", 0, 0, kStartMs);
  TEST_ASSERT_EQUAL(State::Finished, r.state());
  TEST_ASSERT_FALSE(r.word().has_value());
}

void test_whitespace_only_body_is_finished_immediately() {
  const char* body = "   \n  ";
  Reader r(body, std::strlen(body), 0, kStartMs);
  TEST_ASSERT_EQUAL(State::Finished, r.state());
}

void test_resumes_at_saved_word_offset() {
  const char* body = "uno dos tres";
  Reader r(body, std::strlen(body), 4, kStartMs);
  TEST_ASSERT_EQUAL_STRING("dos", shown(r).c_str());
  TEST_ASSERT_EQUAL_UINT(4, r.offset());
}

void test_offset_past_end_restarts_at_first_word() {
  const char* body = "uno dos";
  Reader r(body, std::strlen(body), 999, kStartMs);
  TEST_ASSERT_EQUAL_STRING("uno", shown(r).c_str());
}

void test_offset_inside_utf8_sequence_restarts_at_first_word() {
  const char* body = "ñandú sol";  // ñ is C3 B1; offset 1 is its continuation byte
  Reader r(body, std::strlen(body), 1, kStartMs);
  TEST_ASSERT_EQUAL_UINT(0, r.offset());
  TEST_ASSERT_EQUAL_STRING("ñandú", shown(r).c_str());
}

// --- timing ------------------------------------------------------------------

void test_tick_waits_for_word_period() {
  const char* body = "uno dos";
  Reader r(body, std::strlen(body), 0, kStartMs);
  TEST_ASSERT_FALSE(r.tick(199));
  TEST_ASSERT_TRUE(r.tick(200));
  TEST_ASSERT_EQUAL_STRING("dos", shown(r).c_str());
}

void test_late_tick_keeps_the_planned_pace() {
  const char* body = "uno dos tres";
  Reader r(body, std::strlen(body), 0, kStartMs);
  TEST_ASSERT_TRUE(r.tick(205));  // 5 ms late: "dos" still ends at 400 ms, not 405
  TEST_ASSERT_FALSE(r.tick(399));
  TEST_ASSERT_TRUE(r.tick(400));
}

void test_long_stall_restarts_the_pace_from_now() {
  const char* body = "uno dos tres";
  Reader r(body, std::strlen(body), 0, kStartMs);
  TEST_ASSERT_TRUE(r.tick(1000));  // a full word late: "dos" holds from 1000 ms
  TEST_ASSERT_FALSE(r.tick(1199));
  TEST_ASSERT_TRUE(r.tick(1200));
}

void test_punctuation_holds_the_word_longer() {
  const char* body = "hola, mundo";  // comma holds at 200% = 400 ms
  Reader r(body, std::strlen(body), 0, kStartMs);
  TEST_ASSERT_FALSE(r.tick(399));
  TEST_ASSERT_TRUE(r.tick(400));
  TEST_ASSERT_EQUAL_STRING("mundo", shown(r).c_str());
}

void test_finishes_after_last_word_is_held() {
  const char* body = "uno";
  Reader r(body, std::strlen(body), 0, kStartMs);
  TEST_ASSERT_TRUE(r.tick(200));
  TEST_ASSERT_EQUAL(State::Finished, r.state());
  TEST_ASSERT_EQUAL_UINT(0, r.offset());
}

void test_tick_handles_clock_wraparound() {
  const char* body = "uno dos";
  Reader r(body, std::strlen(body), 0, 0xFFFFFF00u);  // due at 0xFFFFFFC8
  TEST_ASSERT_FALSE(r.tick(0xFFFFFFC7u));
  TEST_ASSERT_TRUE(r.tick(0xFFFFFFC8u));
  TEST_ASSERT_EQUAL_STRING("dos", shown(r).c_str());
}

// --- pause and resume --------------------------------------------------------

void test_paused_reader_does_not_advance() {
  const char* body = "uno dos";
  Reader r(body, std::strlen(body), 0, kStartMs);
  r.pause();
  TEST_ASSERT_EQUAL(State::Paused, r.state());
  TEST_ASSERT_FALSE(r.tick(10000));
  TEST_ASSERT_EQUAL_STRING("uno", shown(r).c_str());
}

void test_resume_holds_current_word_from_resume_time() {
  const char* body = "uno dos";
  Reader r(body, std::strlen(body), 0, kStartMs);
  r.pause();
  r.resume(1000);
  TEST_ASSERT_EQUAL(State::Reading, r.state());
  TEST_ASSERT_FALSE(r.tick(1199));
  TEST_ASSERT_TRUE(r.tick(1200));
}

void test_resume_after_finish_stays_finished() {
  const char* body = "uno";
  Reader r(body, std::strlen(body), 0, kStartMs);
  r.tick(200);
  r.resume(500);
  TEST_ASSERT_EQUAL(State::Finished, r.state());
}

// --- speed -------------------------------------------------------------------

void test_faster_steps_by_25_wpm() {
  const char* body = "uno";
  Reader r(body, std::strlen(body), 0, kStartMs);
  r.faster();
  TEST_ASSERT_EQUAL_UINT(325, r.wpm());
}

void test_faster_clamps_at_800_wpm() {
  const char* body = "uno";
  Reader r(body, std::strlen(body), 0, kStartMs);
  for (int i = 0; i < 40; ++i) r.faster();
  TEST_ASSERT_EQUAL_UINT(800, r.wpm());
}

void test_slower_clamps_at_150_wpm() {
  const char* body = "uno";
  Reader r(body, std::strlen(body), 0, kStartMs);
  for (int i = 0; i < 40; ++i) r.slower();
  TEST_ASSERT_EQUAL_UINT(150, r.wpm());
}

void test_speed_change_applies_to_next_word() {
  const char* body = "uno dos tres";
  Reader r(body, std::strlen(body), 0, kStartMs);
  TEST_ASSERT_TRUE(r.tick(200));  // "dos" shown at 300 WPM, holds 200 ms
  r.slower();                     // 275 WPM: 60000 / 275 rounds to 218 ms
  TEST_ASSERT_FALSE(r.tick(399));
  TEST_ASSERT_TRUE(r.tick(400));  // "tres" is shown at 275 WPM
  TEST_ASSERT_EQUAL_STRING("tres", shown(r).c_str());
  TEST_ASSERT_FALSE(r.tick(617));
  TEST_ASSERT_TRUE(r.tick(618));
  TEST_ASSERT_EQUAL(State::Finished, r.state());
}

// --- seeking while paused ----------------------------------------------------

// --- multibyte text ----------------------------------------------------------

void test_reads_spanish_words_with_multibyte_letters() {
  const char* body = "¡Hola niño!";
  Reader r(body, std::strlen(body), 0, kStartMs);
  TEST_ASSERT_EQUAL_STRING("¡Hola", shown(r).c_str());
  TEST_ASSERT_TRUE(r.tick(200 * 2));  // ¡Hola holds for its own period
  TEST_ASSERT_EQUAL_STRING("niño!", shown(r).c_str());
}

void test_progress_percent_follows_the_shown_word() {
  const char* body = "uno dos. tres";  // 13 bytes; "tres" starts at byte 9
  Reader r(body, std::strlen(body), 0, kStartMs);
  TEST_ASSERT_EQUAL_UINT32(0, r.progress_percent());
  r.pause();
  r.step_sentence(+1);
  TEST_ASSERT_EQUAL_UINT32(69, r.progress_percent());
}


// --- sentence navigation -----------------------------------------------------

void test_sentence_step_forward_and_back_while_paused() {
  const char* body = "Uno dos. Tres cuatro! Cinco";
  Reader r(body, std::strlen(body), 0, kStartMs);
  r.pause();
  r.step_sentence(+1);
  TEST_ASSERT_EQUAL_STRING("Tres", shown(r).c_str());
  r.step_sentence(+1);
  TEST_ASSERT_EQUAL_STRING("Cinco", shown(r).c_str());
  r.step_sentence(-1);
  TEST_ASSERT_EQUAL_STRING("Tres", shown(r).c_str());
  r.step_sentence(-1);
  TEST_ASSERT_EQUAL_STRING("Uno", shown(r).c_str());
}

void test_sentence_forward_stops_at_last_sentence() {
  const char* body = "Uno dos. Tres";
  Reader r(body, std::strlen(body), 0, kStartMs);
  r.pause();
  r.step_sentence(+1);
  r.step_sentence(+1);
  TEST_ASSERT_EQUAL_STRING("Tres", shown(r).c_str());
}

void test_sentence_back_from_mid_sentence_goes_to_its_start() {
  const char* body = "Uno dos. Tres cuatro! Cinco";
  Reader r(body, std::strlen(body), 0, kStartMs);
  r.pause();
  r.step_sentence(+1);      // "Tres"
  r.step_sentence(+1);      // "Cinco"
  r.step_sentence(-1);      // "Tres", not "cuatro!"
  TEST_ASSERT_EQUAL_STRING("Tres", shown(r).c_str());
}

void test_sentence_back_is_stopped_at_book_start() {
  const char* body = "Uno dos. Tres";
  Reader r(body, std::strlen(body), 0, kStartMs);
  r.pause();
  r.step_sentence(-1);
  TEST_ASSERT_EQUAL_STRING("Uno", shown(r).c_str());
}

void test_sentence_step_is_ignored_while_reading() {
  const char* body = "Uno dos. Tres";
  Reader r(body, std::strlen(body), 0, kStartMs);
  r.step_sentence(+1);
  TEST_ASSERT_EQUAL_STRING("Uno", shown(r).c_str());
}

void test_sentence_end_on_paragraph_break_without_period() {
  const char* body = "uno dos\n\ntres cuatro";
  Reader r(body, std::strlen(body), 0, kStartMs);
  r.pause();
  r.step_sentence(+1);
  TEST_ASSERT_EQUAL_STRING("tres", shown(r).c_str());
}

void test_sentence_step_handles_accented_words() {
  const char* body = "¿Qué pasó? ñandú sol.";
  Reader r(body, std::strlen(body), 0, kStartMs);
  r.pause();
  r.step_sentence(+1);
  TEST_ASSERT_EQUAL_STRING("ñandú", shown(r).c_str());
  r.step_sentence(-1);
  TEST_ASSERT_EQUAL_STRING("¿Qué", shown(r).c_str());
}

void test_sentence_forward_is_capped_when_book_has_no_sentence_marks() {
  std::string body;
  for (int i = 0; i < 250; ++i) body += "w" + std::to_string(i) + " ";
  Reader r(body.c_str(), body.size(), 0, kStartMs);
  r.pause();
  r.step_sentence(+1);
  TEST_ASSERT_EQUAL_STRING("w200", shown(r).c_str());
}

// --- pieces (words split by syllable) ----------------------------------------

namespace {

// Splits words of five or more bytes into three pieces; shorter ones stay whole.
std::uint32_t three_pieces_when_long(const char*, std::size_t length) {
  return length >= 5 ? 3 : 1;
}

}  // namespace

void test_piece_advances_one_beat_at_a_time() {
  const char* body = "uno longitud dos";  // beat is 200 ms at 300 WPM
  Reader r(body, std::strlen(body), 0, kStartMs, three_pieces_when_long);
  TEST_ASSERT_TRUE(r.tick(200));          // "longitud" starts on piece 0
  TEST_ASSERT_EQUAL_UINT32(0, r.piece());
  TEST_ASSERT_FALSE(r.tick(399));
  TEST_ASSERT_TRUE(r.tick(400));
  TEST_ASSERT_EQUAL_UINT32(1, r.piece());
  TEST_ASSERT_TRUE(r.tick(600));
  TEST_ASSERT_EQUAL_UINT32(2, r.piece());
  TEST_ASSERT_EQUAL_STRING("longitud", shown(r).c_str());
}

void test_piece_wake_is_next_piece_boundary() {
  const char* body = "uno longitud dos";
  Reader r(body, std::strlen(body), 0, kStartMs, three_pieces_when_long);
  TEST_ASSERT_TRUE(r.tick(200));
  TEST_ASSERT_EQUAL_UINT32(400, r.wake_ms());
  TEST_ASSERT_TRUE(r.tick(400));
  TEST_ASSERT_EQUAL_UINT32(600, r.wake_ms());
}

void test_last_piece_keeps_the_punctuation_hold() {
  const char* body = "longitud, dos";  // three pieces: 400 ms + comma hold of 400 ms
  Reader r(body, std::strlen(body), 0, kStartMs, three_pieces_when_long);
  r.tick(799);  // reaches the last piece, but the word still holds
  TEST_ASSERT_EQUAL_STRING("longitud,", shown(r).c_str());
  TEST_ASSERT_TRUE(r.tick(800));
  TEST_ASSERT_EQUAL_STRING("dos", shown(r).c_str());
}

void test_pause_shows_first_piece_and_resume_restarts_the_word() {
  const char* body = "uno longitud dos";
  Reader r(body, std::strlen(body), 0, kStartMs, three_pieces_when_long);
  r.tick(200);
  r.tick(400);
  TEST_ASSERT_EQUAL_UINT32(1, r.piece());
  r.pause();
  TEST_ASSERT_EQUAL_UINT32(0, r.piece());
  r.resume(1000);
  TEST_ASSERT_EQUAL_UINT32(0, r.piece());
  TEST_ASSERT_TRUE(r.tick(1200));
  TEST_ASSERT_EQUAL_UINT32(1, r.piece());
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_body_offset_points_after_start_marker_line);
  RUN_TEST(test_body_offset_is_nullopt_without_start_marker);
  RUN_TEST(test_body_offset_is_nullopt_for_empty_input);
  RUN_TEST(test_starts_on_first_word_of_body);
  RUN_TEST(test_empty_body_is_finished_immediately);
  RUN_TEST(test_whitespace_only_body_is_finished_immediately);
  RUN_TEST(test_resumes_at_saved_word_offset);
  RUN_TEST(test_offset_past_end_restarts_at_first_word);
  RUN_TEST(test_offset_inside_utf8_sequence_restarts_at_first_word);
  RUN_TEST(test_tick_waits_for_word_period);
  RUN_TEST(test_late_tick_keeps_the_planned_pace);
  RUN_TEST(test_long_stall_restarts_the_pace_from_now);
  RUN_TEST(test_punctuation_holds_the_word_longer);
  RUN_TEST(test_finishes_after_last_word_is_held);
  RUN_TEST(test_tick_handles_clock_wraparound);
  RUN_TEST(test_paused_reader_does_not_advance);
  RUN_TEST(test_resume_holds_current_word_from_resume_time);
  RUN_TEST(test_resume_after_finish_stays_finished);
  RUN_TEST(test_faster_steps_by_25_wpm);
  RUN_TEST(test_faster_clamps_at_800_wpm);
  RUN_TEST(test_slower_clamps_at_150_wpm);
  RUN_TEST(test_speed_change_applies_to_next_word);
  RUN_TEST(test_reads_spanish_words_with_multibyte_letters);
  RUN_TEST(test_progress_percent_follows_the_shown_word);
  RUN_TEST(test_sentence_step_forward_and_back_while_paused);
  RUN_TEST(test_sentence_forward_stops_at_last_sentence);
  RUN_TEST(test_sentence_back_from_mid_sentence_goes_to_its_start);
  RUN_TEST(test_sentence_back_is_stopped_at_book_start);
  RUN_TEST(test_sentence_step_is_ignored_while_reading);
  RUN_TEST(test_sentence_end_on_paragraph_break_without_period);
  RUN_TEST(test_sentence_step_handles_accented_words);
  RUN_TEST(test_sentence_forward_is_capped_when_book_has_no_sentence_marks);
  RUN_TEST(test_piece_advances_one_beat_at_a_time);
  RUN_TEST(test_piece_wake_is_next_piece_boundary);
  RUN_TEST(test_last_piece_keeps_the_punctuation_hold);
  RUN_TEST(test_pause_shows_first_piece_and_resume_restarts_the_word);
  return UNITY_END();
}
