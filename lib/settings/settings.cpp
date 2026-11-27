#include "settings.h"

namespace leeloo::settings {

std::uint8_t step_percent(std::uint8_t value, int direction) {
  const int next = value + direction * kStepPercent;
  if (next < kMinPercent) return kMinPercent;
  if (next > kMaxPercent) return kMaxPercent;
  return static_cast<std::uint8_t>(next);
}

std::uint16_t adjust_reading_step(std::uint16_t value, int direction) {
  constexpr int kCount = sizeof(rsvp::kWpmSteps) / sizeof(rsvp::kWpmSteps[0]);
  const std::uint32_t current = rsvp::normalize_wpm_step(value);
  int index = 0;
  while (index < kCount - 1 && rsvp::kWpmSteps[index] != current) ++index;
  index += direction > 0 ? 1 : (direction < 0 ? -1 : 0);
  index = index < 0 ? 0 : (index > kCount - 1 ? kCount - 1 : index);
  return static_cast<std::uint16_t>(rsvp::kWpmSteps[index]);
}

std::uint8_t display_register(std::uint8_t brightness_percent) {
  // Integer math with +50 rounds half up; the largest product fits in 32 bits.
  return static_cast<std::uint8_t>((255u * brightness_percent + 50u) / 100u);
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
  out.reading_step_ppm = static_cast<std::uint16_t>(rsvp::normalize_wpm_step(raw.reading_step_ppm));
  const auto wifi = static_cast<std::uint8_t>(raw.wifi);
  if (wifi > static_cast<std::uint8_t>(WifiMode::On)) out.wifi = Settings{}.wifi;
  return out;
}

WifiMode cycle_wifi(WifiMode mode, int direction) {
  // Order is Off -> Auto -> On, as listed in DEVICE-MENU.md.
  const int index = static_cast<int>(mode);
  const int next = ((index + direction) % 3 + 3) % 3;
  return static_cast<WifiMode>(next);
}


}  // namespace leeloo::settings
