#include "upload.h"

#include <cstring>
#include <string_view>

namespace leeloo::upload {

namespace {

constexpr std::string_view kTitlePrefix = "TITLE: ";
constexpr std::string_view kAuthorPrefix = "AUTHOR: ";
constexpr std::string_view kPagesPrefix = "PAGES: ";
constexpr std::size_t kMaxPagesDigits = 9;  // keeps the value inside 32 bits

bool starts_with(std::string_view s, std::string_view prefix) {
  return s.substr(0, prefix.size()) == prefix;
}

// `prefix` followed by at least one non-space character.
bool has_value(std::string_view line, std::string_view prefix) {
  if (!starts_with(line, prefix)) return false;
  return line.substr(prefix.size()).find_first_not_of(' ') != std::string_view::npos;
}

bool is_pages_line(std::string_view line) {
  if (!starts_with(line, kPagesPrefix)) return false;
  const std::string_view digits = line.substr(kPagesPrefix.size());
  if (digits.empty() || digits.size() > kMaxPagesDigits) return false;
  return digits.find_first_not_of("0123456789") == std::string_view::npos;
}

int hex_value(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}

bool is_forbidden_name_byte(std::uint8_t b) {
  return b < 0x20 || b == 0x7F || std::strchr("/\\:*?\"<>|", b) != nullptr;
}

}  // namespace

const char* error_code(Result r) {
  switch (r) {
    case Result::Ok: return "";
    case Result::BadName: return "bad_name";
    case Result::LibraryFull: return "library_full";
    case Result::TooLarge: return "too_large";
    case Result::InvalidUtf8: return "invalid_utf8";
    case Result::InvalidFormat: return "invalid_format";
    case Result::NoSpace: return "no_space";
    case Result::StorageError: return "storage_error";
    case Result::Unauthorized: return "unauthorized";
  }
  return "";
}

int http_status(Result r) {
  switch (r) {
    case Result::Ok: return 201;
    case Result::BadName: return 400;
    case Result::LibraryFull: return 409;
    case Result::TooLarge: return 413;
    case Result::InvalidUtf8:
    case Result::InvalidFormat: return 422;
    case Result::NoSpace: return 507;
    case Result::StorageError: return 500;
    case Result::Unauthorized: return 401;
  }
  return 500;
}

bool Utf8Checker::feed(std::uint8_t b) {
  if (need_ > 0) {
    if (b < lo_ || b > hi_) return false;
    lo_ = 0x80;
    hi_ = 0xBF;
    --need_;
    return true;
  }
  if (b < 0x80) return true;
  // The first continuation byte's range depends on the lead byte, which is what
  // rules out overlong forms, surrogates and values above U+10FFFF.
  if (b >= 0xC2 && b <= 0xDF) {
    need_ = 1;
  } else if (b == 0xE0) {
    need_ = 2;
    lo_ = 0xA0;
  } else if ((b >= 0xE1 && b <= 0xEC) || b == 0xEE || b == 0xEF) {
    need_ = 2;
  } else if (b == 0xED) {
    need_ = 2;
    hi_ = 0x9F;
  } else if (b == 0xF0) {
    need_ = 3;
    lo_ = 0x90;
  } else if (b >= 0xF1 && b <= 0xF3) {
    need_ = 3;
  } else if (b == 0xF4) {
    need_ = 3;
    hi_ = 0x8F;
  } else {
    return false;
  }
  return true;
}

Result BookValidator::feed(const std::uint8_t* data, std::size_t len) {
  for (std::size_t i = 0; i < len && error_ == Result::Ok; ++i) {
    error_ = feed_byte(data[i]);
  }
  return error_;
}

Result BookValidator::feed_byte(std::uint8_t b) {
  if (++total_ > kMaxBookBytes) return Result::TooLarge;
  if (b == '\r') return Result::InvalidFormat;  // the format is LF-only
  if (!utf8_.feed(b)) return Result::InvalidUtf8;
  if (state_ == State::Body) {
    ++body_bytes_;
    return Result::Ok;
  }
  if (b == '\n') return end_header_line();
  if (line_len_ >= kMaxHeaderLine) return Result::InvalidFormat;
  line_[line_len_++] = static_cast<char>(b);
  return Result::Ok;
}

Result BookValidator::end_header_line() {
  const std::string_view line(line_, line_len_);
  line_len_ = 0;
  switch (state_) {
    case State::Title:
      if (!has_value(line, kTitlePrefix)) return Result::InvalidFormat;
      state_ = State::Author;
      return Result::Ok;
    case State::Author:
      if (!has_value(line, kAuthorPrefix)) return Result::InvalidFormat;
      state_ = State::PagesOrStart;
      return Result::Ok;
    case State::PagesOrStart:
      if (line == kStartMarker) {
        state_ = State::Body;
        return Result::Ok;
      }
      if (!pages_seen_ && is_pages_line(line)) {
        pages_seen_ = true;
        return Result::Ok;
      }
      return Result::InvalidFormat;
    case State::Body:
      break;
  }
  return Result::InvalidFormat;
}

Result BookValidator::finish() const {
  if (error_ != Result::Ok) return error_;
  if (state_ != State::Body || body_bytes_ == 0) return Result::InvalidFormat;
  if (!utf8_.complete()) return Result::InvalidUtf8;
  return Result::Ok;
}

Result check_capacity(std::size_t book_bytes, std::size_t free_bytes, std::size_t book_count) {
  if (book_count >= kMaxBooks) return Result::LibraryFull;
  // Written as a subtraction so a huge book_bytes cannot overflow the sum.
  if (free_bytes < kReserveBytes || free_bytes - kReserveBytes < book_bytes) return Result::NoSpace;
  return Result::Ok;
}

int percent_decode(const char* in, std::size_t len, char* out, std::size_t cap) {
  if (cap == 0) return -1;
  std::size_t n = 0;
  for (std::size_t i = 0; i < len; ++i) {
    if (n + 1 >= cap) return -1;  // keep room for the NUL
    char c = in[i];
    if (c == '%') {
      if (i + 2 >= len) return -1;  // needs two hex digits after the '%'
      const int hi = hex_value(in[i + 1]);
      const int lo = hex_value(in[i + 2]);
      if (hi < 0 || lo < 0) return -1;
      c = static_cast<char>((hi << 4) | lo);
      i += 2;
    }
    out[n++] = c;
  }
  out[n] = '\0';
  return static_cast<int>(n);
}

std::size_t sanitize_name(const char* in, std::size_t len, char* out, std::size_t cap) {
  if (len == 0 || len > kMaxNameBytes || cap < len + 1) return 0;
  if (in[0] == ' ' || in[len - 1] == ' ') return 0;
  Utf8Checker utf8;
  for (std::size_t i = 0; i < len; ++i) {
    const auto b = static_cast<std::uint8_t>(in[i]);
    if (is_forbidden_name_byte(b) || !utf8.feed(b)) return 0;
  }
  if (!utf8.complete()) return 0;
  std::memcpy(out, in, len);
  out[len] = '\0';
  return len;
}

void format_pin(std::uint32_t random, char* out) {
  std::uint32_t n = random % 10000;
  for (std::size_t i = kPinDigits; i-- > 0;) {
    out[i] = static_cast<char>('0' + n % 10);
    n /= 10;
  }
  out[kPinDigits] = '\0';
}

bool extract_pin(const char* body, std::size_t len, char* out) {
  const std::string_view s(body, len);
  const std::size_t key = s.find("\"pin\"");
  if (key == std::string_view::npos) return false;
  std::size_t i = key + 5;  // past the closing quote of "pin"
  const auto skip_spaces = [&] {
    while (i < s.size() && (s[i] == ' ' || s[i] == '\t' || s[i] == '\n' || s[i] == '\r')) ++i;
  };
  skip_spaces();
  if (i >= s.size() || s[i] != ':') return false;
  ++i;
  skip_spaces();
  if (i >= s.size() || s[i] != '"') return false;
  ++i;
  if (i + kPinDigits >= s.size()) return false;  // digits plus the closing quote must fit
  for (std::size_t d = 0; d < kPinDigits; ++d) {
    if (s[i + d] < '0' || s[i + d] > '9') return false;
    out[d] = s[i + d];
  }
  if (s[i + kPinDigits] != '"') return false;
  out[kPinDigits] = '\0';
  return true;
}

bool pin_matches(const char* expected, const char* given, std::size_t given_len) {
  if (given_len != kPinDigits) return false;
  // No early exit, so timing does not reveal how many digits matched.
  unsigned diff = 0;
  bool expected_ended = false;
  for (std::size_t i = 0; i < kPinDigits; ++i) {
    const char e = expected_ended ? '\0' : expected[i];
    if (e == '\0') expected_ended = true;
    diff |= static_cast<unsigned>(static_cast<std::uint8_t>(e) ^ static_cast<std::uint8_t>(given[i]));
  }
  return diff == 0 && !expected_ended && expected[kPinDigits] == '\0';
}

}  // namespace leeloo::upload
