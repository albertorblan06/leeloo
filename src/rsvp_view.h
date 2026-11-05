#pragma once

#include <cstddef>
#include <cstdint>

#include <U8g2lib.h>

// RSVP word drawing (docs/DESIGN.md): one word at 18 pt with a hollow focal
// letter on the center column, split at syllable boundaries when it does not
// fit. Timing lives in lib/reader; this only draws. Hardware-bound: covered by
// docs/hardware-checklists/reading.md.
namespace leeloo::view {

// Number of pieces the word needs. Words wider than 18 characters are cut with "...".
std::uint32_t piece_count(U8G2& u8g2, const char* word, std::size_t length);

// Draws piece `piece` (0-based, clamped to the last piece). Does not clear or
// send the buffer: the caller owns the frame.
void draw_piece(U8G2& u8g2, const char* word, std::size_t length, std::uint32_t piece);

}  // namespace leeloo::view
