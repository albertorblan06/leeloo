#include <unity.h>

#include <cstring>
#include <string>

#include "upload.h"

using namespace leeloo::upload;

void setUp() {}
void tearDown() {}

namespace {

const char* kValid = "TITLE: T\nAUTHOR: A\nPAGES: 3\n###START###\nhola mundo";

Result validate(const std::string& s) {
  BookValidator v;
  Result r = v.feed(reinterpret_cast<const std::uint8_t*>(s.data()), s.size());
  return r != Result::Ok ? r : v.finish();
}

Result validate_bytewise(const std::string& s) {
  BookValidator v;
  for (char c : s) {
    Result r = v.feed(reinterpret_cast<const std::uint8_t*>(&c), 1);
    if (r != Result::Ok) return r;
  }
  return v.finish();
}

std::string with_body(const std::string& body) {
  return "TITLE: T\nAUTHOR: A\n###START###\n" + body;
}

}  // namespace

// --- book validation ---

void test_accepts_minimal_valid_book() {
  TEST_ASSERT_EQUAL(Result::Ok, validate("TITLE: T\nAUTHOR: A\n###START###\nx"));
}

void test_accepts_book_with_pages() { TEST_ASSERT_EQUAL(Result::Ok, validate(kValid)); }

void test_accepts_spanish_multibyte_content() {
  TEST_ASSERT_EQUAL(Result::Ok,
                    validate("TITLE: Ñandú ¿Qué?\nAUTHOR: José\n###START###\n¡Hola, año!"));
}

void test_bytewise_feed_matches_single_chunk() {
  const std::string s = "TITLE: Ñ\nAUTHOR: Á\nPAGES: 9\n###START###\nañ¿¡ 😀";
  TEST_ASSERT_EQUAL(validate(s), validate_bytewise(s));
  TEST_ASSERT_EQUAL(Result::Ok, validate_bytewise(s));
}

void test_rejects_empty_input() { TEST_ASSERT_EQUAL(Result::InvalidFormat, validate("")); }

void test_rejects_missing_title() {
  TEST_ASSERT_EQUAL(Result::InvalidFormat, validate("AUTHOR: A\n###START###\nx"));
}

void test_rejects_empty_title() {
  TEST_ASSERT_EQUAL(Result::InvalidFormat, validate("TITLE: \nAUTHOR: A\n###START###\nx"));
}

void test_rejects_empty_author() {
  TEST_ASSERT_EQUAL(Result::InvalidFormat, validate("TITLE: T\nAUTHOR: \n###START###\nx"));
}

void test_rejects_author_before_title() {
  TEST_ASSERT_EQUAL(Result::InvalidFormat, validate("AUTHOR: A\nTITLE: T\n###START###\nx"));
}

void test_rejects_missing_start_delimiter() {
  TEST_ASSERT_EQUAL(Result::InvalidFormat, validate("TITLE: T\nAUTHOR: A\nPAGES: 3\nx y z"));
}

void test_rejects_empty_body_after_delimiter() {
  TEST_ASSERT_EQUAL(Result::InvalidFormat, validate("TITLE: T\nAUTHOR: A\n###START###\n"));
  TEST_ASSERT_EQUAL(Result::InvalidFormat, validate("TITLE: T\nAUTHOR: A\n###START###"));
}

void test_rejects_carriage_return() {
  TEST_ASSERT_EQUAL(Result::InvalidFormat, validate("TITLE: T\r\nAUTHOR: A\n###START###\nx"));
  TEST_ASSERT_EQUAL(Result::InvalidFormat, validate(with_body("uno\rdos")));
}

void test_rejects_non_numeric_pages() {
  TEST_ASSERT_EQUAL(Result::InvalidFormat,
                    validate("TITLE: T\nAUTHOR: A\nPAGES: x1\n###START###\nx"));
  TEST_ASSERT_EQUAL(Result::InvalidFormat,
                    validate("TITLE: T\nAUTHOR: A\nPAGES: \n###START###\nx"));
}

void test_rejects_duplicate_pages() {
  TEST_ASSERT_EQUAL(Result::InvalidFormat,
                    validate("TITLE: T\nAUTHOR: A\nPAGES: 1\nPAGES: 2\n###START###\nx"));
}

void test_rejects_header_line_over_limit() {
  const std::string title(kMaxHeaderLine, 'a');
  TEST_ASSERT_EQUAL(Result::InvalidFormat,
                    validate("TITLE: " + title + "\nAUTHOR: A\n###START###\nx"));
}

void test_accepts_header_line_at_limit() {
  const std::string title(kMaxHeaderLine - 7, 'a');  // "TITLE: " is 7 bytes
  TEST_ASSERT_EQUAL(Result::Ok, validate("TITLE: " + title + "\nAUTHOR: A\n###START###\nx"));
}

void test_rejects_lone_utf8_continuation_byte() {
  TEST_ASSERT_EQUAL(Result::InvalidUtf8, validate(with_body("a\x80z")));
}

void test_rejects_overlong_utf8() {
  TEST_ASSERT_EQUAL(Result::InvalidUtf8, validate(with_body("\xC0\x80")));
  TEST_ASSERT_EQUAL(Result::InvalidUtf8, validate(with_body("\xE0\x80\x80")));
}

void test_rejects_utf8_surrogate() {
  TEST_ASSERT_EQUAL(Result::InvalidUtf8, validate(with_body("\xED\xA0\x80")));
}

void test_rejects_utf8_above_unicode_range() {
  TEST_ASSERT_EQUAL(Result::InvalidUtf8, validate(with_body("\xF4\x90\x80\x80")));
}

void test_rejects_truncated_utf8_at_end() {
  TEST_ASSERT_EQUAL(Result::InvalidUtf8, validate(with_body("a\xC3")));  // ñ cut in half
}

void test_accepts_book_exactly_at_size_limit() {
  const std::string head = "TITLE: T\nAUTHOR: A\n###START###\n";
  TEST_ASSERT_EQUAL(Result::Ok, validate(head + std::string(kMaxBookBytes - head.size(), 'a')));
}

void test_rejects_book_one_byte_over_size_limit() {
  const std::string head = "TITLE: T\nAUTHOR: A\n###START###\n";
  TEST_ASSERT_EQUAL(Result::TooLarge,
                    validate(head + std::string(kMaxBookBytes - head.size() + 1, 'a')));
}

void test_first_error_is_sticky() {
  BookValidator v;
  const std::string bad = "NOPE\n";
  TEST_ASSERT_EQUAL(Result::InvalidFormat,
                    v.feed(reinterpret_cast<const std::uint8_t*>(bad.data()), bad.size()));
  const std::string good = "TITLE: T\n";
  TEST_ASSERT_EQUAL(Result::InvalidFormat,
                    v.feed(reinterpret_cast<const std::uint8_t*>(good.data()), good.size()));
  TEST_ASSERT_EQUAL(Result::InvalidFormat, v.finish());
}

// --- capacity ---

void test_capacity_ok_with_room() {
  TEST_ASSERT_EQUAL(Result::Ok, check_capacity(1000, kReserveBytes + 1000, 0));
}

void test_capacity_rejects_when_reserve_would_be_used() {
  TEST_ASSERT_EQUAL(Result::NoSpace, check_capacity(1000, kReserveBytes + 999, 0));
}

void test_capacity_rejects_when_free_below_reserve() {
  TEST_ASSERT_EQUAL(Result::NoSpace, check_capacity(1, kReserveBytes - 1, 0));
}

void test_capacity_rejects_full_library() {
  TEST_ASSERT_EQUAL(Result::LibraryFull, check_capacity(1, kReserveBytes * 10, kMaxBooks));
}

void test_capacity_accepts_last_free_slot() {
  TEST_ASSERT_EQUAL(Result::Ok, check_capacity(1, kReserveBytes * 10, kMaxBooks - 1));
}

void test_capacity_does_not_overflow_with_huge_book() {
  TEST_ASSERT_EQUAL(Result::NoSpace, check_capacity(static_cast<std::size_t>(-1), kReserveBytes * 2, 0));
}

// --- names ---

void test_percent_decode_decodes_utf8_escapes() {
  char out[32];
  TEST_ASSERT_EQUAL_INT(3, percent_decode("%C3%B1a", 7, out, sizeof out));
  TEST_ASSERT_EQUAL_STRING("\xC3\xB1" "a", out);
}

void test_percent_decode_rejects_bad_escape() {
  char out[32];
  TEST_ASSERT_EQUAL_INT(-1, percent_decode("%zz", 3, out, sizeof out));
  TEST_ASSERT_EQUAL_INT(-1, percent_decode("%4", 2, out, sizeof out));
}

void test_percent_decode_rejects_small_buffer() {
  char out[3];
  TEST_ASSERT_EQUAL_INT(-1, percent_decode("abcd", 4, out, sizeof out));
}

void test_sanitize_accepts_spanish_name() {
  char out[kMaxNameBytes + 1];
  const char* name = "Don Quijote \xC3\xB1";
  TEST_ASSERT_EQUAL_UINT(std::strlen(name), sanitize_name(name, std::strlen(name), out, sizeof out));
  TEST_ASSERT_EQUAL_STRING(name, out);
}

void test_sanitize_rejects_path_separators_and_controls() {
  char out[kMaxNameBytes + 1];
  TEST_ASSERT_EQUAL_UINT(0, sanitize_name("a/b", 3, out, sizeof out));
  TEST_ASSERT_EQUAL_UINT(0, sanitize_name("..\\x", 4, out, sizeof out));
  TEST_ASSERT_EQUAL_UINT(0, sanitize_name("a\nb", 3, out, sizeof out));
  TEST_ASSERT_EQUAL_UINT(0, sanitize_name("a:b", 3, out, sizeof out));
}

void test_sanitize_rejects_empty_and_blank() {
  char out[kMaxNameBytes + 1];
  TEST_ASSERT_EQUAL_UINT(0, sanitize_name("", 0, out, sizeof out));
  TEST_ASSERT_EQUAL_UINT(0, sanitize_name("   ", 3, out, sizeof out));
}

void test_sanitize_rejects_surrounding_spaces() {
  char out[kMaxNameBytes + 1];
  TEST_ASSERT_EQUAL_UINT(0, sanitize_name(" a", 2, out, sizeof out));
  TEST_ASSERT_EQUAL_UINT(0, sanitize_name("a ", 2, out, sizeof out));
}

void test_sanitize_accepts_name_at_byte_limit() {
  char out[kMaxNameBytes + 1];
  const std::string name(kMaxNameBytes, 'a');
  TEST_ASSERT_EQUAL_UINT(kMaxNameBytes, sanitize_name(name.data(), name.size(), out, sizeof out));
}

void test_sanitize_rejects_name_over_byte_limit() {
  char out[kMaxNameBytes + 1];
  const std::string name(kMaxNameBytes + 1, 'a');
  TEST_ASSERT_EQUAL_UINT(0, sanitize_name(name.data(), name.size(), out, sizeof out));
}

void test_sanitize_counts_bytes_not_characters() {
  char out[kMaxNameBytes + 1];
  std::string name;
  for (std::size_t i = 0; i < kMaxNameBytes / 2 + 1; ++i) name += "\xC3\xB1";  // 2 bytes each
  TEST_ASSERT_EQUAL_UINT(0, sanitize_name(name.data(), name.size(), out, sizeof out));
}

void test_sanitize_rejects_invalid_utf8() {
  char out[kMaxNameBytes + 1];
  TEST_ASSERT_EQUAL_UINT(0, sanitize_name("a\xC3", 2, out, sizeof out));
}

void test_sanitize_rejects_small_output_buffer() {
  char out[4];
  TEST_ASSERT_EQUAL_UINT(0, sanitize_name("abcdef", 6, out, sizeof out));
}

// --- PIN ---

void test_format_pin_pads_with_zeros() {
  char pin[kPinDigits + 1];
  format_pin(7, pin);
  TEST_ASSERT_EQUAL_STRING("0007", pin);
}

void test_format_pin_wraps_modulo_ten_thousand() {
  char pin[kPinDigits + 1];
  format_pin(123456, pin);
  TEST_ASSERT_EQUAL_STRING("3456", pin);
}

void test_pin_matches_exact_value() {
  TEST_ASSERT_TRUE(pin_matches("1234", "1234", 4));
}

void test_pin_rejects_wrong_value_or_length() {
  TEST_ASSERT_FALSE(pin_matches("1234", "1235", 4));
  TEST_ASSERT_FALSE(pin_matches("1234", "123", 3));
  TEST_ASSERT_FALSE(pin_matches("1234", "12345", 5));
  TEST_ASSERT_FALSE(pin_matches("1234", "", 0));
}

void test_extract_pin_reads_the_web_app_body() {
  char pin[kPinDigits + 1];
  const char* body = "{\"pin\":\"0427\"}";
  TEST_ASSERT_TRUE(extract_pin(body, std::strlen(body), pin));
  TEST_ASSERT_EQUAL_STRING("0427", pin);
}

void test_extract_pin_tolerates_whitespace() {
  char pin[kPinDigits + 1];
  const char* body = " { \"pin\" :  \"1234\" } ";
  TEST_ASSERT_TRUE(extract_pin(body, std::strlen(body), pin));
  TEST_ASSERT_EQUAL_STRING("1234", pin);
}

void test_extract_pin_rejects_malformed_bodies() {
  char pin[kPinDigits + 1];
  const char* bad[] = {"", "{}", "{\"pin\":1234}", "{\"pin\":\"123\"}", "{\"pin\":\"12345\"}",
                       "{\"pin\":\"12a4\"}", "{\"pin\":\"1234", "{\"pin\" \"1234\"}", "pin"};
  for (const char* b : bad) {
    TEST_ASSERT_FALSE_MESSAGE(extract_pin(b, std::strlen(b), pin), b);
  }
}

// --- error mapping (docs/WEB-API.md §3.2) ---

void test_error_codes_and_statuses_match_contract() {
  TEST_ASSERT_EQUAL_STRING("bad_name", error_code(Result::BadName));
  TEST_ASSERT_EQUAL_INT(400, http_status(Result::BadName));
  TEST_ASSERT_EQUAL_STRING("library_full", error_code(Result::LibraryFull));
  TEST_ASSERT_EQUAL_INT(409, http_status(Result::LibraryFull));
  TEST_ASSERT_EQUAL_STRING("too_large", error_code(Result::TooLarge));
  TEST_ASSERT_EQUAL_INT(413, http_status(Result::TooLarge));
  TEST_ASSERT_EQUAL_STRING("invalid_utf8", error_code(Result::InvalidUtf8));
  TEST_ASSERT_EQUAL_INT(422, http_status(Result::InvalidUtf8));
  TEST_ASSERT_EQUAL_STRING("invalid_format", error_code(Result::InvalidFormat));
  TEST_ASSERT_EQUAL_INT(422, http_status(Result::InvalidFormat));
  TEST_ASSERT_EQUAL_STRING("no_space", error_code(Result::NoSpace));
  TEST_ASSERT_EQUAL_INT(507, http_status(Result::NoSpace));
  TEST_ASSERT_EQUAL_STRING("storage_error", error_code(Result::StorageError));
  TEST_ASSERT_EQUAL_INT(500, http_status(Result::StorageError));
  TEST_ASSERT_EQUAL_STRING("unauthorized", error_code(Result::Unauthorized));
  TEST_ASSERT_EQUAL_INT(401, http_status(Result::Unauthorized));
  TEST_ASSERT_EQUAL_STRING("", error_code(Result::Ok));
  TEST_ASSERT_EQUAL_INT(201, http_status(Result::Ok));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_accepts_minimal_valid_book);
  RUN_TEST(test_accepts_book_with_pages);
  RUN_TEST(test_accepts_spanish_multibyte_content);
  RUN_TEST(test_bytewise_feed_matches_single_chunk);
  RUN_TEST(test_rejects_empty_input);
  RUN_TEST(test_rejects_missing_title);
  RUN_TEST(test_rejects_empty_title);
  RUN_TEST(test_rejects_empty_author);
  RUN_TEST(test_rejects_author_before_title);
  RUN_TEST(test_rejects_missing_start_delimiter);
  RUN_TEST(test_rejects_empty_body_after_delimiter);
  RUN_TEST(test_rejects_carriage_return);
  RUN_TEST(test_rejects_non_numeric_pages);
  RUN_TEST(test_rejects_duplicate_pages);
  RUN_TEST(test_rejects_header_line_over_limit);
  RUN_TEST(test_accepts_header_line_at_limit);
  RUN_TEST(test_rejects_lone_utf8_continuation_byte);
  RUN_TEST(test_rejects_overlong_utf8);
  RUN_TEST(test_rejects_utf8_surrogate);
  RUN_TEST(test_rejects_utf8_above_unicode_range);
  RUN_TEST(test_rejects_truncated_utf8_at_end);
  RUN_TEST(test_accepts_book_exactly_at_size_limit);
  RUN_TEST(test_rejects_book_one_byte_over_size_limit);
  RUN_TEST(test_first_error_is_sticky);
  RUN_TEST(test_capacity_ok_with_room);
  RUN_TEST(test_capacity_rejects_when_reserve_would_be_used);
  RUN_TEST(test_capacity_rejects_when_free_below_reserve);
  RUN_TEST(test_capacity_rejects_full_library);
  RUN_TEST(test_capacity_accepts_last_free_slot);
  RUN_TEST(test_capacity_does_not_overflow_with_huge_book);
  RUN_TEST(test_percent_decode_decodes_utf8_escapes);
  RUN_TEST(test_percent_decode_rejects_bad_escape);
  RUN_TEST(test_percent_decode_rejects_small_buffer);
  RUN_TEST(test_sanitize_accepts_spanish_name);
  RUN_TEST(test_sanitize_rejects_path_separators_and_controls);
  RUN_TEST(test_sanitize_rejects_empty_and_blank);
  RUN_TEST(test_sanitize_rejects_surrounding_spaces);
  RUN_TEST(test_sanitize_accepts_name_at_byte_limit);
  RUN_TEST(test_sanitize_rejects_name_over_byte_limit);
  RUN_TEST(test_sanitize_counts_bytes_not_characters);
  RUN_TEST(test_sanitize_rejects_invalid_utf8);
  RUN_TEST(test_sanitize_rejects_small_output_buffer);
  RUN_TEST(test_format_pin_pads_with_zeros);
  RUN_TEST(test_format_pin_wraps_modulo_ten_thousand);
  RUN_TEST(test_pin_matches_exact_value);
  RUN_TEST(test_pin_rejects_wrong_value_or_length);
  RUN_TEST(test_extract_pin_reads_the_web_app_body);
  RUN_TEST(test_extract_pin_tolerates_whitespace);
  RUN_TEST(test_extract_pin_rejects_malformed_bodies);
  RUN_TEST(test_error_codes_and_statuses_match_contract);
  return UNITY_END();
}
