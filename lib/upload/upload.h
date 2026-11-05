#pragma once

#include <cstddef>
#include <cstdint>

// Pure logic for the Upload Book session (docs/WEB-API.md). No Arduino, no I/O:
// the HTTP/Wi-Fi/storage adapter in src/ feeds bytes in and acts on the results.
namespace leeloo::upload {

// Mirror docs/WEB-API.md §4 and web/src/limits.js. Initial values, not measured.
constexpr std::size_t kMaxBookBytes = 1048576;
constexpr std::size_t kMaxBooks = 20;
constexpr std::size_t kMaxNameBytes = 48;
constexpr std::size_t kReserveBytes = 2097152;
constexpr std::size_t kPinDigits = 4;
// A header line (TITLE/AUTHOR/PAGES) longer than this is rejected.
constexpr std::size_t kMaxHeaderLine = 512;

enum class Result {
  Ok,
  BadName,
  LibraryFull,
  TooLarge,
  InvalidUtf8,
  InvalidFormat,
  NoSpace,
  StorageError,  // the temp-file write or the final rename failed
  Unauthorized,  // missing or unknown session cookie
};

// Error code string and HTTP status from docs/WEB-API.md §3.2. Ok maps to
// "" and 201.
const char* error_code(Result r);
int http_status(Result r);

// Strict streaming UTF-8 checker: rejects overlong forms, surrogates and code
// points above U+10FFFF. Call feed() per byte; complete() is true on a code
// point boundary.
class Utf8Checker {
 public:
  bool feed(std::uint8_t b);
  bool complete() const { return need_ == 0; }

 private:
  std::uint8_t need_ = 0;  // continuation bytes still expected
  std::uint8_t lo_ = 0x80;  // allowed range of the next continuation byte
  std::uint8_t hi_ = 0xBF;
};

// Streaming validator for the single-file book format (docs/ARCHITECTURE.md
// §5.1). Bytes are fed in chunks as they arrive, so the whole book never has
// to sit in RAM. The first error is sticky: later feed() calls return it.
class BookValidator {
 public:
  // Returns Ok while the data so far is acceptable.
  Result feed(const std::uint8_t* data, std::size_t len);
  // Call after the last chunk. Returns Ok only for a complete, valid book.
  Result finish() const;

 private:
  enum class State { Title, Author, PagesOrStart, Body };

  Result feed_byte(std::uint8_t b);
  Result end_header_line();

  Result error_ = Result::Ok;
  State state_ = State::Title;
  Utf8Checker utf8_;
  std::size_t total_ = 0;
  std::size_t body_bytes_ = 0;
  bool pages_seen_ = false;
  char line_[kMaxHeaderLine] = {};
  std::size_t line_len_ = 0;
};

// Library capacity: Ok, LibraryFull or NoSpace. `free_bytes` is what the
// filesystem reports; the firmware reserve is kept untouched.
Result check_capacity(std::size_t book_bytes, std::size_t free_bytes, std::size_t book_count);

// Decodes %XX escapes from the X-Book-Name header into `out` (NUL-terminated).
// Returns the decoded length, or -1 on a bad escape or if `cap` is too small.
int percent_decode(const char* in, std::size_t len, char* out, std::size_t cap);

// Validates a decoded book name: valid UTF-8, no control characters or path
// separators (/ \ : * ? " < > |), no surrounding spaces, 1..kMaxNameBytes
// bytes. Copies it into `out` (NUL-terminated, cap >= kMaxNameBytes + 1).
// Returns the length, or 0 when the name is rejected.
std::size_t sanitize_name(const char* in, std::size_t len, char* out, std::size_t cap);

// Writes a zero-padded PIN of kPinDigits digits plus NUL into `out`
// (needs kPinDigits + 1 bytes). The modulo bias is negligible for a PIN.
void format_pin(std::uint32_t random, char* out);

// Reads the PIN from the POST /api/auth JSON body, e.g. {"pin":"0427"}, into
// `out` (kPinDigits + 1 bytes). Not a general JSON parser: it accepts only a
// "pin" key with a string of exactly kPinDigits digits. False otherwise.
bool extract_pin(const char* body, std::size_t len, char* out);

// Constant-time comparison; false on any length mismatch.
bool pin_matches(const char* expected, const char* given, std::size_t given_len);

}  // namespace leeloo::upload
