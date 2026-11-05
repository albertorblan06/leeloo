#include "settings.h"

namespace leeloo::settings {

std::uint8_t step_percent(std::uint8_t value, int direction) {
  const int next = value + direction * kStepPercent;
  if (next < kMinPercent) return kMinPercent;
  if (next > kMaxPercent) return kMaxPercent;
  return static_cast<std::uint8_t>(next);
}

std::uint8_t display_register(std::uint8_t brightness_percent, std::uint8_t contrast_percent) {
  // Integer math with +5000 rounds half up; the largest product fits in 32 bits.
  const std::uint32_t scaled = 255u * brightness_percent * contrast_percent;
  return static_cast<std::uint8_t>((scaled + 5000u) / 10000u);
}

namespace {

std::uint8_t snap_percent(std::uint8_t value) {
  const int clamped = value < kMinPercent ? kMinPercent : (value > kMaxPercent ? kMaxPercent : value);
  const int snapped = (clamped + kStepPercent / 2) / kStepPercent * kStepPercent;
  return static_cast<std::uint8_t>(snapped < kMinPercent ? kMinPercent : snapped);
}

}  // namespace

Settings sanitize(Settings raw) {
  Settings out = raw;
  out.brightness_percent = snap_percent(raw.brightness_percent);
  out.contrast_percent = snap_percent(raw.contrast_percent);
  const auto wifi = static_cast<std::uint8_t>(raw.wifi);
  if (wifi > static_cast<std::uint8_t>(WifiMode::On)) out.wifi = Settings{}.wifi;
  const auto mode = static_cast<std::uint8_t>(raw.brightness_mode);
  if (mode > static_cast<std::uint8_t>(BrightnessMode::Manual)) out.brightness_mode = Settings{}.brightness_mode;
  return out;
}

WifiMode cycle_wifi(WifiMode mode, int direction) {
  // Order is Off -> Auto -> On, as listed in DEVICE-MENU.md.
  const int index = static_cast<int>(mode);
  const int next = ((index + direction) % 3 + 3) % 3;
  return static_cast<WifiMode>(next);
}

BrightnessMode cycle_brightness(BrightnessMode mode) {
  return mode == BrightnessMode::Auto ? BrightnessMode::Manual : BrightnessMode::Auto;
}

}  // namespace leeloo::settings
