#pragma once

#include <cstddef>
#include <cstdint>

// OLED rendering of one RSVP word (hardware-bound; see docs/hardware-checklists/).
namespace leeloo::view {

// Starts I2C and the display. Returns false when no OLED answers at 0x3C/0x3D.
bool begin(int sda_pin, int scl_pin);

// Shows a word at `wpm`, split at syllable boundaries until it fits, for
// `hold_percent` of a word period (100 = one normal beat). Blocks while shown.
void show_word(const char* word, std::size_t length, std::uint32_t wpm, std::uint32_t hold_percent);

void clear();

}  // namespace leeloo::view
