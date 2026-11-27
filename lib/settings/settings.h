#pragma once

#include <cstdint>

#include "rsvp.h"

// Persistent device settings (docs/DEVICE-MENU.md §2 Ajustes). Pure logic: the
// NVS adapter lives in src/ and only moves these values to and from storage.
namespace leeloo::settings {

enum class WifiMode : std::uint8_t { Off, Auto, On };

// Percentages are 10..100 in steps of 5 (DEVICE-MENU.md §2).
constexpr std::uint8_t kMinPercent = 10;
constexpr std::uint8_t kMaxPercent = 100;
constexpr std::uint8_t kStepPercent = 5;

struct Settings {
  // "Brillo" in the menu. The SH1106 has no separate backlight, so this drives its contrast
  // register directly (display_register); there is no light sensor, hence no Auto mode.
  std::uint8_t brightness_percent = kMaxPercent;
  // Step of the live speed change while reading (rsvp::kWpmSteps). The last-used
  // speed itself is not here: RsvpService owns and saves it (ISpeedStore).
  std::uint16_t reading_step_ppm = static_cast<std::uint16_t>(rsvp::kDefaultWpmStep);
  // Automático keeps the AP off until Biblioteca > Añadir libro or Grabadora > Enviar al móvil. The spec does not set a default.
  WifiMode wifi = WifiMode::Auto;
};

// Moves a percentage by one step (direction +1 or -1), clamped to the range.
std::uint8_t step_percent(std::uint8_t value, int direction);

// Moves the reading step to the next (+1) or previous (-1) entry of rsvp::kWpmSteps,
// clamped at both ends.
std::uint16_t adjust_reading_step(std::uint16_t value, int direction);

// SH1106 contrast register for a brightness percentage: round(255 * percent / 100). The mapping
// is an approximation; the real useful range is an open hardware check in docs/DEVICE-MENU.md §5.
std::uint8_t display_register(std::uint8_t brightness_percent);

// Makes a value read back from storage safe to use: the percentage is clamped and rounded to the
// 5% grid, and an unknown Wi-Fi value falls back to the default.
Settings sanitize(Settings raw);

WifiMode cycle_wifi(WifiMode mode, int direction);

}  // namespace leeloo::settings
