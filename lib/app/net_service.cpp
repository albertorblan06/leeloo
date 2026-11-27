#include "net_service.h"

#include <algorithm>

namespace leeloo::app {

namespace {

std::uint32_t until(std::uint32_t at, std::uint32_t now) {
  const auto delta = static_cast<std::int32_t>(at - now);
  return delta <= 0 ? 0 : static_cast<std::uint32_t>(delta);
}

}  // namespace

NetService::NetService(INet* net, rtos::Queue<ui::Event>* ui_out, ILog* log)
    : net_(net), ui_out_(ui_out), log_(log) {}

// Stops the AP unless the On setting keeps it up while the device is awake.
void NetService::end_upload_session() {
  auto_session_ = false;
  recording_session_ = false;
  timer_.stop();
  stop_at_ms_ = 0;
  if (applied_wifi_ != settings::WifiMode::On) net_->stop();
}

void NetService::start_upload(std::uint32_t now_ms, bool recording) {
  stop_at_ms_ = 0;  // a deferred stop from an earlier save must not end this session
  recording_session_ = recording;
  if (!net_->running() && !net_->start()) {
    if (log_ != nullptr) log_->log("Wi-Fi: no se pudo iniciar");
    post(ui_out_, recording ? ui::Event::AudioUploadStartFailed : ui::Event::UploadStartFailed, log_,
         "Cola de UI llena: se pierde el aviso de fallo de Wi-Fi");
    recording_session_ = false;
    return;
  }
  auto_session_ = applied_wifi_ == settings::WifiMode::Auto;
  if (auto_session_) {
    // An AP that was already up may carry an old activity time; restart the window now.
    net_->mark_activity();
    timer_.start(now_ms);
  }
}

void NetService::handle(const NetMsg& msg, std::uint32_t now_ms) {
  switch (msg.cmd) {
    case NetCmd::Boot:
      applied_wifi_ = msg.wifi;
      if (msg.wifi == settings::WifiMode::On && !net_->start() && log_ != nullptr) {
        log_->log("Wi-Fi: no se pudo iniciar al arrancar");
      }
      return;
    case NetCmd::StartUpload:
      start_upload(now_ms, false);
      return;
    case NetCmd::StartRecordingUpload:
      start_upload(now_ms, true);
      return;
    case NetCmd::StopUpload:
    case NetCmd::StopRecordingUpload:
      end_upload_session();
      return;
    case NetCmd::ApplyWifi:
      // Wi-Fi changes apply immediately; Automático waits for Biblioteca > Añadir libro. Leaving On
      // stops an AP that no session owns, unless the upload screen is open on it.
      if (msg.wifi == settings::WifiMode::On && !net_->running()) net_->start();
      if (msg.wifi != settings::WifiMode::On && net_->running() && !msg.upload_screen_open) {
        net_->stop();
      }
      applied_wifi_ = msg.wifi;
      return;
    case NetCmd::SavedOnUploadScreen:
      // Only a save made from the upload screen schedules the stop; otherwise the session is already over.
      if (applied_wifi_ != settings::WifiMode::On) stop_at_ms_ = now_ms + kStopAfterSaveMs;
      return;
    case NetCmd::RecordingTransferred:
      // Same grace as a saved book: the phone still has its answer to receive.
      if (applied_wifi_ != settings::WifiMode::On) stop_at_ms_ = now_ms + kStopAfterSaveMs;
      return;
  }
}

// Auto sessions end after the inactivity limit; the phone's requests keep them alive.
void NetService::on_timeout(std::uint32_t now_ms) {
  if (stop_at_ms_ != 0 && static_cast<std::int32_t>(now_ms - stop_at_ms_) >= 0) {
    end_upload_session();
    return;
  }
  if (!auto_session_ || !net_->running()) return;
  timer_.touch(net_->last_activity_ms());
  if (timer_.expired(now_ms)) {
    if (log_ != nullptr) log_->log("Wi-Fi: sesión cerrada por inactividad");
    const ui::Event timed_out =
        recording_session_ ? ui::Event::AudioUploadTimedOut : ui::Event::UploadTimedOut;
    end_upload_session();
    post(ui_out_, timed_out, log_, "Cola de UI llena: se pierde el aviso de inactividad");
  }
}

std::uint32_t NetService::wait_ms(std::uint32_t now_ms) const {
  std::uint32_t wait = rtos::kWaitForever;
  if (stop_at_ms_ != 0) wait = until(stop_at_ms_, now_ms);
  if (auto_session_) wait = std::min(wait, kInactivityPollMs);
  return wait;
}

}  // namespace leeloo::app
