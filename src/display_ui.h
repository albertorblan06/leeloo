#pragma once

#include <cstddef>
#include <cstdint>

#include "reading_frame.h"
#include "recording_frame.h"
#include "ui.h"

// OLED adapter for the device menu. Draws the grid from docs/DESIGN.md §3 for the
// state held by leeloo::ui::Ui. Hardware-bound: covered by docs/hardware-checklists/menu.md.
namespace leeloo::display {

// Live values the screens show next to the menu state.
struct Status {
  bool storage_ok = false;
  std::size_t free_bytes = 0;
  bool ap_running = false;
  const char* pin = "";
  app::RecordingFrame recording;
};

// Starts the I2C bus and the panel. Returns false when no OLED answers at 0x3C/0x3D.
bool init();
bool ready();

// Draws the next frame of the boot logo (docs/DESIGN.md §4) for the time since the first call and
// returns true once the animation has finished (or the panel is not there). Call it once, then
// do the slow start-up work, then call it until it returns true. Before the tasks start.
bool boot_animation_step();

// Draws one frame. Called by the display task only: it owns the I2C bus. The
// reading frame is used on the Reading screen and ignored elsewhere. Changing screen
// slides the old frame out over ~200 ms, never into or out of Reading.
void render(const ui::Ui& ui, const Status& status, const app::ReadingFrame& frame);

// Pieces a word needs at the RSVP size. Passed to the reader; 1 when the OLED is not up.
// Called from the RSVP task: it shares the font with render() through an internal lock.
std::uint32_t piece_count(const char* word, std::size_t length);

// Turns the panel off before deep sleep. Call from the display task, which owns the I2C bus.
void power_save();

// Applies the single SH1106 contrast register (see settings::display_register).
void set_contrast(std::uint8_t register_value);

}  // namespace leeloo::display
