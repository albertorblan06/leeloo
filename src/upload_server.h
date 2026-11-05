#pragma once

#include <cstdint>

// Hardware adapter for the Upload Book session (docs/WEB-API.md): soft AP, HTTP
// server, LittleFS. All decisions live in lib/upload; this only moves bytes.
namespace leeloo::upload_server {

// Fixed AP name and address shown on the OLED (the ESP32 soft AP default address).
constexpr const char* kSsid = "Leeloo";
constexpr const char* kAddress = "192.168.4.1";

// Starts the soft AP and the HTTP server and writes the new session PIN into
// `pin_out` (needs leeloo::upload::kPinDigits + 1 bytes). Returns false, with
// everything it started stopped again, when storage is not mounted or Wi-Fi/HTTP
// fail to start. Requires storage::mount() to have succeeded first.
bool start(char* pin_out);

// Stops the server and the AP and forgets the PIN and session.
void stop();

bool running();
const char* current_pin();

// Millisecond timestamp of the last request, for the Auto-mode inactivity timer.
std::uint32_t last_activity_ms();
// Counts as activity now; used when a session restarts on an AP that is already up.
void mark_activity();

// Called from the HTTP task after a book is stored. Keep it short: it runs there.
void set_on_saved(void (*callback)());

}  // namespace leeloo::upload_server
