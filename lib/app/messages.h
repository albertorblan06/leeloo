#pragma once

#include <cstddef>
#include <cstdint>

#include "settings.h"

// Messages that cross between tasks. Every type is trivially copyable, because
// queues move them as raw bytes (lib/rtos).
namespace leeloo::app {

// Posting waits this long for room in a full queue before the message is dropped and logged.
constexpr std::uint32_t kPostTimeoutMs = 100;

constexpr std::size_t kButtonCount = 3;
enum Button : std::uint8_t { kUp = 0, kDown = 1, kSelect = 2 };

// One level change of a button pin, stamped in the ISR. `high` is true when released (pull-up).
struct RawEdge {
  std::uint8_t button;
  bool high;
  std::uint32_t ms;
};

// UiService -> RsvpService.
enum class RsvpCmd : std::uint8_t { Open, Close, Select, Up, Down };
struct RsvpMsg {
  RsvpCmd cmd;
  std::size_t index;  // Open: library index of the book
};

// UiService -> NetService.
enum class NetCmd : std::uint8_t {
  Boot,                 // apply the saved Wi-Fi mode once at start
  StartUpload,          // the upload screen opened
  StopUpload,           // the upload screen closed
  ApplyWifi,            // the Wi-Fi setting changed
  SavedOnUploadScreen,  // a book was stored while the upload screen was open
};
struct NetMsg {
  NetCmd cmd;
  settings::WifiMode wifi;  // Boot and ApplyWifi
  bool upload_screen_open;  // ApplyWifi: the screen owns the running AP
};

}  // namespace leeloo::app
