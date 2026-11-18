#include "settings.h"

#include <algorithm>

namespace leeloo::settings {

std::uint8_t step_percent(std::uint8_t value, int direction) {
  const int next = value + direction * kStepPercent;
  if (next < kMinPercent) return kMinPercent;
  if (next > kMaxPercent) return kMaxPercent;
  return static_cast<std::uint8_t>(next);
}

std::uint16_t adjust_reading_wpm(std::uint16_t value, int direction, std::uint16_t step_ppm) {
  const std::uint32_t step = rsvp::normalize_wpm_step(step_ppm);
  const std::uint32_t current = rsvp::normalize_wpm(value, step);
  if (direction > 0) return static_cast<std::uint16_t>(std::min(current + step, rsvp::kMaxWpm));
  if (direction < 0) return static_cast<std::uint16_t>(std::max(current - step, rsvp::kMinWpm));
  return static_cast<std::uint16_t>(current);
}

std::uint16_t adjust_reading_step(std::uint16_t value, int direction) {
  constexpr std::size_t kStepCount = sizeof(rsvp::kWpmSteps) / sizeof(rsvp::kWpmSteps[0]);
  const std::uint32_t current = rsvp::normalize_wpm_step(value);
  std::size_t index = 0;
  while (index < kStepCount && rsvp::kWpmSteps[index] != current) ++index;
  if (index == kStepCount) return kDefaultReadingStepPpm;
  if (direction > 0 && index + 1 < kStepCount) ++index;
  if (direction < 0 && index > 0) --index;
  return static_cast<std::uint16_t>(rsvp::kWpmSteps[index]);
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
  out.reading_step_ppm =
      static_cast<std::uint16_t>(rsvp::normalize_wpm_step(raw.reading_step_ppm));
  out.last_reading_wpm = static_cast<std::uint16_t>(
      rsvp::normalize_wpm(raw.last_reading_wpm, out.reading_step_ppm));
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
