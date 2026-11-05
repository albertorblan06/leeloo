#include "ui_service.h"

namespace leeloo::app {

UiService::UiService(const settings::Settings& initial, ILibrary* library, ISettingsStore* store,
                     IDisplayControl* display, rtos::Queue<RsvpMsg>* rsvp_out,
                     rtos::Queue<NetMsg>* net_out, ILog* log)
    : ui_(initial),
      library_(library),
      store_(store),
      display_(display),
      rsvp_out_(rsvp_out),
      net_out_(net_out),
      log_(log) {}

void UiService::sync_books() {
  const std::size_t count = library_->book_count();
  if (count != ui_.book_count()) ui_.set_book_count(count);
}

void UiService::handle(ui::Event event) {
  // Only a save made from the upload screen can schedule a stop. The screen is read
  // before the event is handled, because the event itself leaves the upload screen.
  if (event == ui::Event::UploadSaved && ui_.screen() == ui::Screen::Upload) {
    post(net_out_, NetMsg{NetCmd::SavedOnUploadScreen, settings::WifiMode::Auto, false}, log_,
         "Cola de red llena: se pierde SavedOnUploadScreen");
  }
  execute(ui_.handle(event));
  // Always refresh after a save, even when the count is unchanged: a replace keeps
  // the count but changes what the indexes point at.
  if (event == ui::Event::UploadSaved) ui_.set_book_count(library_->book_count());
}

void UiService::execute(const ui::Outcome& out) {
  const auto to_rsvp = [this](RsvpCmd cmd, std::size_t index) {
    post(rsvp_out_, RsvpMsg{cmd, index}, log_, "Cola de lectura llena: se pierde una orden");
  };
  const auto to_net = [this](NetCmd cmd) {
    post(net_out_, NetMsg{cmd, settings::WifiMode::Auto, false}, log_,
         "Cola de red llena: se pierde una orden");
  };

  switch (out.command) {
    case ui::Command::None:
      return;
    case ui::Command::OpenBook:
      to_rsvp(RsvpCmd::Open, out.index);
      return;
    case ui::Command::CloseBook:
      to_rsvp(RsvpCmd::Close, 0);
      return;
    case ui::Command::ReadingSelect:
      to_rsvp(RsvpCmd::Select, 0);
      return;
    case ui::Command::ReadingUp:
      to_rsvp(RsvpCmd::Up, 0);
      return;
    case ui::Command::ReadingDown:
      to_rsvp(RsvpCmd::Down, 0);
      return;
    case ui::Command::StartUpload:
      to_net(NetCmd::StartUpload);
      return;
    case ui::Command::StopUpload:
      to_net(NetCmd::StopUpload);
      return;
    case ui::Command::DeleteBook:
      if (!library_->remove_at(out.index)) handle(ui::Event::DeleteFailed);
      sync_books();
      return;
    case ui::Command::SaveSettings: {
      const settings::Settings& s = ui_.settings();
      if (!store_->save(s) && log_ != nullptr) log_->log("Ajustes: no se pudo guardar");
      display_->set_contrast(settings::display_register(s.brightness_percent, s.contrast_percent));
      post(net_out_, NetMsg{NetCmd::ApplyWifi, s.wifi, ui_.screen() == ui::Screen::Upload}, log_,
           "Cola de red llena: se pierde ApplyWifi");
      return;
    }
  }
}

}  // namespace leeloo::app
