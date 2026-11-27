#include "ui.h"

namespace leeloo::ui {

std::size_t Ui::item_count() const {
  switch (screen_) {
    case Screen::Root: return kRootItems;
    case Screen::Read:
    case Screen::DeleteList: return book_count_ + 1;  // books, then Atrás
    case Screen::Manage: return 3;                    // Añadir libro, Borrar libro, Atrás
    case Screen::DeleteConfirm: return 2;  // Cancelar, Borrar
    case Screen::ReadEnd: return 2;        // Releer, Volver al menú
    case Screen::Settings: return 4;       // Brillo, Salto por pulsación, Wi-Fi, Atrás
    case Screen::Brightness:
    case Screen::Wifi:
    case Screen::SpeedStep: return 2;  // the value row, Atrás
    case Screen::RecordMenu:
      return !audio_storage_ready_ ? 1 : (recording_available_ ? 3 : 2);  // ..., Atrás
    case Screen::Recording:
    case Screen::RecordingDeleteConfirm:
    case Screen::PowerConfirm: return 2;  // Pausar/Reanudar + Detener; Cancelar + Borrar
    case Screen::Upload:
    case Screen::RecordingUpload:
    case Screen::About: return 1;      // Atrás only
    case Screen::Reading:
    case Screen::PoweringOff:
    case Screen::Message: return 0;
  }
  return 0;
}

// Screens whose last row is a selectable `< Atrás` (DEVICE-MENU §3).
bool Ui::has_back_item() const {
  switch (screen_) {
    case Screen::Read:
    case Screen::Manage:
    case Screen::Upload:
    case Screen::RecordMenu:
    case Screen::RecordingUpload:
    case Screen::DeleteList:
    case Screen::Settings:
    case Screen::Brightness:
    case Screen::Wifi:
    case Screen::SpeedStep:
    case Screen::About:
      return true;
    default:
      return false;
  }
}

void Ui::enter_back(Screen screen) {
  entering_back_ = true;
  enter(screen);
  entering_back_ = false;
}

void Ui::enter(Screen screen) {
  if (screen != screen_) back_navigation_ = entering_back_;
  screen_ = screen;
  editing_ = false;
  cursor_ = menu::Menu(item_count());
}

void Ui::show_message(Message message) {
  return_to_ = screen_;
  message_ = message;
  enter(Screen::Message);
}

void Ui::set_book_count(std::size_t count) {
  book_count_ = count;
  // The pending index may now point at another book, so the confirmation is dropped.
  if (screen_ == Screen::DeleteConfirm) enter_back(Screen::DeleteList);
  if (screen_ == Screen::Read || screen_ == Screen::DeleteList) enter(screen_);
}

void Ui::set_recording_available(bool available) {
  recording_available_ = available;
  if (screen_ == Screen::RecordMenu) enter(Screen::RecordMenu);  // the rows change with it
}

void Ui::set_audio_storage_ready(bool ready) {
  audio_storage_ready_ = ready;
  if (screen_ == Screen::RecordMenu) enter(Screen::RecordMenu);
}

Outcome Ui::handle(Event event) {
  // Every audio event is declared after AudioAvailable in ui.h.
  if (screen_ == Screen::PoweringOff) return {};  // the device is about to sleep: nothing can interrupt it
  if (event >= Event::AudioAvailable) return on_audio_event(event);
  if (screen_ == Screen::Upload) return on_upload_event(event);
  if (screen_ == Screen::Recording) return on_recording_event(event);
  if (screen_ == Screen::RecordingUpload) return on_recording_upload_event(event);
  if (screen_ == Screen::Reading) return on_reading_event(event);
  if (event == Event::UpRepeat || event == Event::DownRepeat) return on_repeat(event);
  if (screen_ == Screen::Message) {
    if (event == Event::Select || event == Event::Back) enter_back(return_to_);
    return {};
  }
  if (event == Event::DeleteFailed) {
    if (screen_ == Screen::DeleteList) show_message(Message::DeleteFailed);
    return {};
  }
  switch (event) {
    case Event::Up:
    case Event::Down:
      if (editing_) return change(event == Event::Up ? +1 : -1);
      move(event);
      return {};
    case Event::Left: return change(-1);
    case Event::Right: return change(+1);
    case Event::Select: return select();
    case Event::Back: return back();
    default: return {};
  }
}

// Held-button repeats move values, never the cursor: menus step once per press.
Outcome Ui::on_repeat(Event event) {
  if (!editing_) return {};
  const int direction = event == Event::UpRepeat ? +1 : -1;
  switch (screen_) {
    case Screen::Brightness:
    case Screen::SpeedStep:
      return change(direction);
    default:
      return {};
  }
}

void Ui::move(Event event) {
  if (event == Event::Up) {
    cursor_.up();
  } else {
    cursor_.down();
  }
}

Outcome Ui::on_upload_event(Event event) {
  // Every upload result leads back to Biblioteca, where the user started it.
  switch (event) {
    case Event::Back:
    case Event::Select:  // the `< Atrás` row, the only item on this screen
      enter_back(Screen::Manage);
      return {Command::StopUpload, 0};
    case Event::UploadStartFailed:
      enter_back(Screen::Manage);
      show_message(Message::ApStartFailed);
      return {};
    case Event::UploadSaved:
      enter_back(Screen::Manage);
      show_message(Message::UploadSaved);
      return {};
    case Event::UploadTimedOut:
      enter_back(Screen::Manage);
      show_message(Message::UploadTimedOut);
      return {};
    default:
      return {};
  }
}

// Audio events can arrive on any screen: the phone may confirm a transfer while the user
// reads, and the recorder reports at boot. Availability is therefore updated first and
// the screen only decides what to show.
Outcome Ui::on_audio_event(Event event) {
  const auto fail_to_menu = [this](Screen from, Message message) {
    if (screen_ != from) return;
    enter_back(Screen::RecordMenu);
    show_message(message);
  };
  switch (event) {
    case Event::AudioAvailable:
      set_recording_available(true);
      return {};
    case Event::AudioStorageUnavailable:
      set_audio_storage_ready(false);
      return {};
    case Event::AudioSaved:
      set_recording_available(true);
      fail_to_menu(Screen::Recording, Message::AudioSaved);
      return {};
    case Event::AudioDeleted:
      set_recording_available(false);
      fail_to_menu(Screen::RecordingDeleteConfirm, Message::AudioDeleted);
      return {};
    case Event::AudioDeleteFailed:
      fail_to_menu(Screen::RecordingDeleteConfirm, Message::AudioDeleteFailed);
      return {};
    case Event::AudioTransferred:
      set_recording_available(false);
      if (screen_ != Screen::RecordingUpload) return {};
      fail_to_menu(Screen::RecordingUpload, Message::AudioTransferred);
      return {Command::RecordingTransferred, 0};
    case Event::AudioStartFailed:
      fail_to_menu(Screen::Recording, Message::AudioStartFailed);
      return {};
    case Event::AudioBusy:
      fail_to_menu(Screen::Recording, Message::AudioBusy);
      return {};
    case Event::AudioNoSpace:
      fail_to_menu(Screen::Recording, Message::AudioNoSpace);
      return {};
    case Event::AudioSaveFailed:
      fail_to_menu(Screen::Recording, Message::AudioSaveFailed);
      return {};
    case Event::AudioUploadStartFailed:
      fail_to_menu(Screen::RecordingUpload, Message::AudioUploadStartFailed);
      return {};
    case Event::AudioUploadTimedOut:
      fail_to_menu(Screen::RecordingUpload, Message::AudioUploadTimedOut);
      return {};
    case Event::RecordingEnded:  // a Stop found no recording: do not leave the user on its screen
      if (screen_ == Screen::Recording) enter_back(Screen::RecordMenu);
      return {};
    default:
      return {};
  }
}

// While recording, UP/DOWN pick Pausar/Reanudar or Detener. Stopping is asked of the
// recorder and the screen leaves when it answers, so it always shows the real state.
Outcome Ui::on_recording_event(Event event) {
  switch (event) {
    case Event::Up:
    case Event::Down:
      move(event);
      return {};
    case Event::Select:
      return {cursor() == 0 ? Command::ToggleRecordingPause : Command::StopRecording, 0};
    case Event::Back:
      return {Command::StopRecording, 0};
    default:
      return {};
  }
}

// Same shape as the book upload screen: its only item is `< Atrás`.
Outcome Ui::on_recording_upload_event(Event event) {
  switch (event) {
    case Event::Back:
    case Event::Select:
      enter_back(Screen::RecordMenu);
      return {Command::StopRecordingUpload, 0};
    default:
      return {};
  }
}

// While reading, the buttons belong to playback. The adapter decides what each one
// does, since UP/DOWN mean speed or seek depending on whether the reader is paused.
Outcome Ui::on_reading_event(Event event) {
  switch (event) {
    case Event::Up: return {Command::ReadingUp, 0};
    case Event::Down: return {Command::ReadingDown, 0};
    case Event::UpRepeat: return {Command::ReadingUpRepeat, 0};
    case Event::DownRepeat: return {Command::ReadingDownRepeat, 0};
    case Event::Select: return {Command::ReadingSelect, 0};
    case Event::Back:
      enter(Screen::Read);
      return {Command::CloseBook, 0};
    case Event::ReaderFinished:
      enter(Screen::ReadEnd);
      return {};
    case Event::BookOpenFailed:
      enter(Screen::Read);
      show_message(Message::BookOpenFailed);
      return {};
    default:
      return {};
  }
}

Outcome Ui::select() {
  if (back_focused()) return back();
  switch (screen_) {
    case Screen::Root:
      switch (cursor()) {
        case 0: enter(Screen::Read); break;
        case 1: enter(Screen::Manage); break;
        case 2: enter(Screen::RecordMenu); break;
        case 3: enter(Screen::Settings); break;
        default: enter(Screen::About); break;
      }
      return {};
    case Screen::Read:
      if (book_count_ == 0) return {};
      reading_book_ = cursor();  // read before enter() resets the cursor
      enter(Screen::Reading);
      return {Command::OpenBook, reading_book_};
    case Screen::ReadEnd:
      if (cursor() == 0) {  // Releer: the adapter finds no saved position and starts at 0
        enter(Screen::Reading);
        return {Command::OpenBook, reading_book_};
      }
      enter_back(Screen::Read);
      return {Command::CloseBook, 0};
    case Screen::Manage:
      if (cursor() == 1) {
        enter(Screen::DeleteList);
        return {};
      }
      if (settings_.wifi == settings::WifiMode::Off) {
        show_message(Message::WifiOff);
        return {};
      }
      enter(Screen::Upload);
      return {Command::StartUpload, 0};
    case Screen::RecordMenu:
      if (!recording_available_) {  // Nueva grabación
        enter(Screen::Recording);
        return {Command::StartRecording, 0};
      }
      if (cursor() == 1) {  // Borrar grabación
        enter(Screen::RecordingDeleteConfirm);
        return {};
      }
      if (settings_.wifi == settings::WifiMode::Off) {  // Enviar al móvil
        show_message(Message::WifiOff);
        return {};
      }
      enter(Screen::RecordingUpload);
      return {Command::StartRecordingUpload, 0};
    case Screen::PowerConfirm:
      if (cursor() == 0) {  // No
        enter_back(Screen::Root);
        return {};
      }
      enter(Screen::PoweringOff);  // the display task sees this screen and sleeps
      return {};
    case Screen::RecordingDeleteConfirm:
      if (cursor() == 0) {  // Cancelar
        enter_back(Screen::RecordMenu);
        return {};
      }
      return {Command::DeleteRecording, 0};  // stays until AudioDeleted or AudioDeleteFailed
    case Screen::DeleteList:
      if (book_count_ == 0) return {};
      pending_book_ = cursor();
      enter(Screen::DeleteConfirm);
      return {};
    case Screen::DeleteConfirm:
      if (cursor() == 0) {  // Cancelar
        enter_back(Screen::DeleteList);
        return {};
      }
      enter_back(Screen::DeleteList);
      return {Command::DeleteBook, pending_book_};
    case Screen::Settings:
      if (cursor() == 0) enter(Screen::Brightness);
      if (cursor() == 1) enter(Screen::SpeedStep);
      if (cursor() == 2) enter(Screen::Wifi);
      return {};
    case Screen::Brightness:
    case Screen::Wifi:
    case Screen::SpeedStep:
      editing_ = !editing_;
      return {};
    default:
      return {};
  }
}

Outcome Ui::change(int direction) {
  if (back_focused()) return {};  // the Atrás row has no value
  switch (screen_) {
    case Screen::Brightness: {
      const std::uint8_t next = settings::step_percent(settings_.brightness_percent, direction);
      if (next == settings_.brightness_percent) return {};
      settings_.brightness_percent = next;
      return {Command::SaveSettings, 0};
    }
    case Screen::Wifi:
      settings_.wifi = settings::cycle_wifi(settings_.wifi, direction);
      return {Command::SaveSettings, 0};
    case Screen::SpeedStep: {
      const std::uint16_t next = settings::adjust_reading_step(settings_.reading_step_ppm, direction);
      if (next == settings_.reading_step_ppm) return {};
      settings_.reading_step_ppm = next;
      return {Command::SaveSettings, 0};
    }
    default:
      return {};
  }
}

Outcome Ui::back() {
  if (editing_) {  // the first Back only ends the edit
    editing_ = false;
    return {};
  }
  switch (screen_) {
    case Screen::Root:  // nowhere to go back to: a long press here is the way to switch off
      enter(Screen::PowerConfirm);
      return {};
    case Screen::PowerConfirm:
      enter_back(Screen::Root);
      return {};
    case Screen::Read:
    case Screen::Manage:
    case Screen::RecordMenu:
    case Screen::Settings:
    case Screen::About:
      enter_back(Screen::Root);
      return {};
    case Screen::Recording:
      return {Command::StopRecording, 0};
    case Screen::RecordingDeleteConfirm:
      enter_back(Screen::RecordMenu);
      return {};
    case Screen::DeleteList:
      enter_back(Screen::Manage);
      return {};
    case Screen::DeleteConfirm:
      enter_back(Screen::DeleteList);
      return {};
    case Screen::ReadEnd:
      enter_back(Screen::Read);
      return {Command::CloseBook, 0};
    case Screen::Brightness:
    case Screen::Wifi:
    case Screen::SpeedStep:
      enter_back(Screen::Settings);
      return {};
    case Screen::Message:
      enter_back(return_to_);
      return {};
    default:
      return {};
  }
}

}  // namespace leeloo::ui
