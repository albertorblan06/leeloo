#pragma once

#include <cstdint>

#include "inactivity.h"
#include "messages.h"
#include "ports.h"
#include "rtos.h"
#include "settings.h"
#include "ui.h"

namespace leeloo::app {

// Owns the Wi-Fi upload session: when the AP starts and stops, the inactivity limit
// of an Automático session, and the short grace time after a save. UiService sends
// commands; this service tells the UI when a start fails or a session times out.
// The net task blocks for wait_ms() on its command queue, calls handle() for each
// command and on_timeout() when the wait runs out.
class NetService {
 public:
  // The AP stays up this long after a save so the phone receives its 201.
  static constexpr std::uint32_t kStopAfterSaveMs = 1500;
  // How often an Automático session reads the server's last-activity time.
  // shortcut: polling; upgrade to a message from the HTTP task on each request if it costs power.
  static constexpr std::uint32_t kInactivityPollMs = 500;

  NetService(INet* net, rtos::Queue<ui::Event>* ui_out, ILog* log = nullptr);

  void handle(const NetMsg& msg, std::uint32_t now_ms);
  void on_timeout(std::uint32_t now_ms);

  // kWaitForever when nothing is pending; otherwise the time to the next check.
  std::uint32_t wait_ms(std::uint32_t now_ms) const;

 private:
  void start_upload(std::uint32_t now_ms);
  void end_upload_session();

  INet* net_;
  rtos::Queue<ui::Event>* ui_out_;
  ILog* log_;
  upload::InactivityTimer timer_{upload::kUploadInactivityMs};
  settings::WifiMode applied_wifi_ = settings::WifiMode::Auto;
  bool auto_session_ = false;  // Automático: the session ends after inactivity
  std::uint32_t stop_at_ms_ = 0;  // deferred stop after a save; 0 means none
};

}  // namespace leeloo::app
