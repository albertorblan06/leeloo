#pragma once

// Hardware adapter for the Upload Book session (docs/WEB-API.md): soft AP, HTTP
// server, LittleFS. All decisions live in lib/upload; this only moves bytes.
namespace leeloo::upload_server {

// Starts the soft AP and the HTTP server and writes the new session PIN into
// `pin_out` (needs leeloo::upload::kPinDigits + 1 bytes). Returns false, with
// everything it started stopped again, if the filesystem cannot be mounted or
// Wi-Fi/HTTP fail to start. Never formats the filesystem.
bool start(char* pin_out);

// Stops the server and the AP and forgets the PIN and session.
void stop();

}  // namespace leeloo::upload_server
