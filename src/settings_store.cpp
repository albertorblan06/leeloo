#include "settings_store.h"

#include <Preferences.h>

namespace leeloo::settings_store {
namespace {

constexpr const char* kNamespace = "leeloo";
// "bright" replaces the old "bmode", "blevel" and "contrast" keys, which are no longer read.
constexpr const char* kKeyBrightness = "bright";
constexpr const char* kKeyWifi = "wifi";
constexpr const char* kKeyWpmStep = "wpmstep";
constexpr const char* kKeyWpm = "wpm";

}  // namespace

leeloo::settings::Settings load() {
  using leeloo::settings::Settings;
  Preferences prefs;
  Settings s;
  if (!prefs.begin(kNamespace, true)) return s;  // read-only open; defaults if absent
  s.brightness_percent = prefs.getUChar(kKeyBrightness, s.brightness_percent);
  s.wifi = static_cast<leeloo::settings::WifiMode>(
      prefs.getUChar(kKeyWifi, static_cast<std::uint8_t>(s.wifi)));
  s.reading_step_ppm = prefs.getUShort(kKeyWpmStep, s.reading_step_ppm);
  prefs.end();
  return leeloo::settings::sanitize(s);
}

bool save(const leeloo::settings::Settings& settings) {
  Preferences prefs;
  if (!prefs.begin(kNamespace, false)) return false;
  bool ok = true;
  ok &= prefs.putUChar(kKeyBrightness, settings.brightness_percent) > 0;
  ok &= prefs.putUChar(kKeyWifi, static_cast<std::uint8_t>(settings.wifi)) > 0;
  ok &= prefs.putUShort(kKeyWpmStep, settings.reading_step_ppm) > 0;
  prefs.end();
  return ok;
}

std::uint16_t load_reading_wpm() {
  Preferences prefs;
  std::uint16_t wpm = leeloo::rsvp::kDefaultWpm;
  if (!prefs.begin(kNamespace, true)) return wpm;
  wpm = prefs.getUShort(kKeyWpm, wpm);
  prefs.end();
  // Any stored value is aligned to a valid step by the reader; here only the range is enforced.
  return static_cast<std::uint16_t>(leeloo::rsvp::normalize_wpm(wpm, 5));
}

bool save_reading_wpm(std::uint16_t wpm) {
  Preferences prefs;
  if (!prefs.begin(kNamespace, false)) return false;
  const bool ok = prefs.putUShort(kKeyWpm, wpm) > 0;
  prefs.end();
  return ok;
}

}  // namespace leeloo::settings_store
