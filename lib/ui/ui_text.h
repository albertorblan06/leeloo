#pragma once

#include "settings.h"
#include "ui.h"

// User-visible Spanish text that is more than a label, kept in lib/ so tests can check that
// every message exists and fits the OLED row (128 px / 6 px = 21 characters).
namespace leeloo::ui {

constexpr int kMaxRowChars = 21;

struct MessageText {
  const char* line1;
  const char* line2;  // may be empty
};

// Two lines for each message screen. Message::None has empty lines.
MessageText message_text(Message message);

// What the Wi-Fi mode does, under its value on the Wi-Fi screen.
MessageText wifi_help(settings::WifiMode mode);

}  // namespace leeloo::ui
