#pragma once

#include <cstddef>
#include <cstdint>

#include "ui.h"

namespace leeloo::reader {
class Reader;
}

// OLED adapter for the device menu. Draws the grid from docs/DESIGN.md §3 for the
// state held by leeloo::ui::Ui. Hardware-bound: covered by docs/hardware-checklists/menu.md.
namespace leeloo::display {

// Live values the screens show next to the menu state.
struct Status {
  bool storage_ok = false;
  std::size_t free_bytes = 0;
  bool ap_running = false;
  const char* pin = "";
  const reader::Reader* reader = nullptr;  // the book being read, when the Reading screen is open
  bool show_wpm = false;                   // the WPM label is shown after a speed change
};

// Starts the I2C bus and the panel. Returns false when no OLED answers at 0x3C/0x3D.
bool init();
bool ready();

void render(const ui::Ui& ui, const Status& status);

// Pieces a word needs at the RSVP size. Passed to the reader; 1 when the OLED is not up.
std::uint32_t piece_count(const char* word, std::size_t length);

// Applies the single SH1106 contrast register (see settings::display_register).
void set_contrast(std::uint8_t register_value);

}  // namespace leeloo::display
