#pragma once

#include <cstdint>

// Persistent device settings (docs/DEVICE-MENU.md §2 Ajustes). Pure logic: the
// NVS adapter lives in src/ and only moves these values to and from storage.
namespace leeloo::settings {

enum class BrightnessMode : std::uint8_t { Auto, Manual };
enum class WifiMode : std::uint8_t { Off, Auto, On };

// Percentages are 10..100 in steps of 5 (DEVICE-MENU.md §2).
constexpr std::uint8_t kMinPercent = 10;
constexpr std::uint8_t kMaxPercent = 100;
constexpr std::uint8_t kStepPercent = 5;

struct Settings {
  BrightnessMode brightness_mode = BrightnessMode::Manual;
  std::uint8_t brightness_percent = kMaxPercent;
  std::uint8_t contrast_percent = kMaxPercent;
  // Automático keeps the AP off until Biblioteca > Subir. The spec does not set a default.
  WifiMode wifi = WifiMode::Auto;
};

// Moves a percentage by one step (direction +1 or -1), clamped to the range.
std::uint8_t step_percent(std::uint8_t value, int direction);

// Single SH1106 contrast register from the brightness and contrast percentages:
// round(255 * brightness * contrast / 100²). The mapping is an approximation; the
// real useful range is an open hardware check in docs/DEVICE-MENU.md §5.
std::uint8_t display_register(std::uint8_t brightness_percent, std::uint8_t contrast_percent);

// Makes a value read back from storage safe to use: percentages are clamped and
// rounded to the 5% grid, and an unknown Wi-Fi value falls back to the default.
Settings sanitize(Settings raw);

WifiMode cycle_wifi(WifiMode mode, int direction);
BrightnessMode cycle_brightness(BrightnessMode mode);

}  // namespace leeloo::settings
