#include "text.h"

#include <cstdint>
#include <cstring>

namespace leeloo::text {
namespace {

bool is_continuation(std::uint8_t b) { return (b & 0xC0) == 0x80; }

// Byte offset just past the first `n` code points of s (or the end of s).
std::size_t offset_after_chars(const char* s, std::size_t len, std::size_t n) {
  std::size_t i = 0;
  for (std::size_t seen = 0; i < len && seen < n; ++seen) {
    ++i;
    while (i < len && is_continuation(static_cast<std::uint8_t>(s[i]))) ++i;
  }
  return i;
}

std::size_t count_chars(const char* s, std::size_t len) {
  std::size_t n = 0;
  for (std::size_t i = 0; i < len; ++i) {
    if (!is_continuation(static_cast<std::uint8_t>(s[i]))) ++n;
  }
  return n;
}

}  // namespace

bool valid_utf8(const char* s, std::size_t len) {
  std::size_t i = 0;
  while (i < len) {
    const auto lead = static_cast<std::uint8_t>(s[i]);
    std::size_t need = 0;
    std::uint8_t lo = 0x80;
    std::uint8_t hi = 0xBF;
    if (lead < 0x80) {
      ++i;
      continue;
    }
    if (lead >= 0xC2 && lead <= 0xDF) {
      need = 1;
    } else if (lead == 0xE0) {
      need = 2;
      lo = 0xA0;
    } else if ((lead >= 0xE1 && lead <= 0xEC) || lead == 0xEE || lead == 0xEF) {
      need = 2;
    } else if (lead == 0xED) {
      need = 2;
      hi = 0x9F;
    } else if (lead == 0xF0) {
      need = 3;
      lo = 0x90;
    } else if (lead >= 0xF1 && lead <= 0xF3) {
      need = 3;
    } else if (lead == 0xF4) {
      need = 3;
      hi = 0x8F;
    } else {
      return false;
    }
    if (i + need >= len) return false;  // truncated sequence at the end
    for (std::size_t k = 1; k <= need; ++k) {
      const auto b = static_cast<std::uint8_t>(s[i + k]);
      const std::uint8_t first_lo = k == 1 ? lo : 0x80;
      const std::uint8_t first_hi = k == 1 ? hi : 0xBF;
      if (b < first_lo || b > first_hi) return false;
    }
    i += need + 1;
  }
  return true;
}

std::size_t truncate_chars(const char* text, std::size_t len, std::size_t max_chars,
                           const char* ellipsis, char* out, std::size_t cap) {
  if (cap == 0) return 0;
  out[0] = '\0';
  if (!valid_utf8(text, len)) return 0;

  const std::size_t ellipsis_len = std::strlen(ellipsis);
  const std::size_t ellipsis_chars = count_chars(ellipsis, ellipsis_len);

  std::size_t written = 0;
  if (count_chars(text, len) <= max_chars) {
    written = len;
    if (written + 1 > cap) return 0;
    std::memcpy(out, text, len);
  } else if (max_chars <= ellipsis_chars) {
    written = offset_after_chars(ellipsis, ellipsis_len, max_chars);
    if (written + 1 > cap) return 0;
    std::memcpy(out, ellipsis, written);
  } else {
    const std::size_t keep = offset_after_chars(text, len, max_chars - ellipsis_chars);
    written = keep + ellipsis_len;
    if (written + 1 > cap) return 0;
    std::memcpy(out, text, keep);
    std::memcpy(out + keep, ellipsis, ellipsis_len);
  }
  out[written] = '\0';
  return written;
}

std::size_t scroll_start(std::size_t previous_start, std::size_t cursor, std::size_t count,
                         std::size_t rows) {
  if (count <= rows) return 0;
  const std::size_t max_start = count - rows;
  std::size_t start = previous_start > max_start ? max_start : previous_start;
  if (cursor < start) {
    start = cursor;
  } else if (cursor >= start + rows) {
    start = cursor + 1 - rows;
  }
  return start;
}

}  // namespace leeloo::text
