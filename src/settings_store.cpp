#include "settings_store.h"

#include <Preferences.h>

namespace leeloo::settings_store {
namespace {

constexpr const char* kNamespace = "leeloo";
constexpr const char* kKeyBrightnessMode = "bmode";
constexpr const char* kKeyBrightness = "blevel";
constexpr const char* kKeyContrast = "contrast";
constexpr const char* kKeyWifi = "wifi";

}  // namespace

leeloo::settings::Settings load() {
  using leeloo::settings::Settings;
  Preferences prefs;
  Settings s;
  if (!prefs.begin(kNamespace, true)) return s;  // read-only open; defaults if absent
  s.brightness_mode = static_cast<leeloo::settings::BrightnessMode>(
      prefs.getUChar(kKeyBrightnessMode, static_cast<std::uint8_t>(s.brightness_mode)));
  s.brightness_percent = prefs.getUChar(kKeyBrightness, s.brightness_percent);
  s.contrast_percent = prefs.getUChar(kKeyContrast, s.contrast_percent);
  s.wifi = static_cast<leeloo::settings::WifiMode>(
      prefs.getUChar(kKeyWifi, static_cast<std::uint8_t>(s.wifi)));
  prefs.end();
  return leeloo::settings::sanitize(s);
}

bool save(const leeloo::settings::Settings& settings) {
  Preferences prefs;
  if (!prefs.begin(kNamespace, false)) return false;
  bool ok = true;
  ok &= prefs.putUChar(kKeyBrightnessMode, static_cast<std::uint8_t>(settings.brightness_mode)) > 0;
  ok &= prefs.putUChar(kKeyBrightness, settings.brightness_percent) > 0;
  ok &= prefs.putUChar(kKeyContrast, settings.contrast_percent) > 0;
  ok &= prefs.putUChar(kKeyWifi, static_cast<std::uint8_t>(settings.wifi)) > 0;
  prefs.end();
  return ok;
}

}  // namespace leeloo::settings_store
