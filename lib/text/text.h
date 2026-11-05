#pragma once

#include <cstddef>

// Text helpers for the OLED lists (docs/DEVICE-MENU.md §4). Pure logic, no display code.
namespace leeloo::text {

// True when `len` bytes form valid UTF-8 (no overlong forms, surrogates or
// code points above U+10FFFF). Same rules as the upload validator.
bool valid_utf8(const char* s, std::size_t len);

// Copies `text` into `out` (NUL-terminated), cut to at most `max_chars` code points.
// A cut text ends with `ellipsis` and never splits a code point. If `max_chars` is
// not larger than the ellipsis, only its first `max_chars` characters are written.
// Returns the bytes written, excluding the NUL, or 0 on invalid UTF-8 or when `cap`
// cannot hold the result.
std::size_t truncate_chars(const char* text, std::size_t len, std::size_t max_chars,
                           const char* ellipsis, char* out, std::size_t cap);

// First visible row for a list of `count` items shown in `rows` rows, given the
// previous first row and the cursor. Keeps the cursor visible, moves only as far
// as needed, and never leaves empty rows at the bottom.
std::size_t scroll_start(std::size_t previous_start, std::size_t cursor, std::size_t count,
                         std::size_t rows);

}  // namespace leeloo::text
