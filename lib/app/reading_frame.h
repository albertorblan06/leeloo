#pragma once

#include <cstddef>
#include <cstdint>

#include "reader.h"

namespace leeloo::app {

// Wider words are cut with "...". Same limit the OLED view applies (src/rsvp_view.cpp).
constexpr std::size_t kFrameWordChars = 18;
// 18 code points of up to 4 bytes, plus NUL.
constexpr std::size_t kFrameWordCap = 80;

// What the display needs to draw one reading frame. It holds a copy of the word,
// never a pointer into the book text, so the book can be closed while the frame
// waits to be drawn.
struct ReadingFrame {
  bool active = false;  // false when no book is open
  reader::State state = reader::State::Reading;
  std::uint32_t wpm = 0;
  std::uint32_t piece = 0;
  bool show_wpm = false;
  char word[kFrameWordCap] = {};
};

}  // namespace leeloo::app
