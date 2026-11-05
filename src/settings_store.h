#pragma once

#include "settings.h"

// Persists the Ajustes values in NVS (Preferences, built into the Arduino core).
namespace leeloo::settings_store {

// Reads the saved settings, or the defaults when nothing is stored yet. Values
// are sanitized, so a damaged entry cannot put the menu out of range.
leeloo::settings::Settings load();

// Writes all values. Returns false when NVS rejects the write.
bool save(const leeloo::settings::Settings& settings);

}  // namespace leeloo::settings_store
