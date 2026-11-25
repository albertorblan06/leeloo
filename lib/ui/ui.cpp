#include "ui.h"

namespace leeloo::ui {

std::size_t Ui::item_count() const {
  switch (screen_) {
    case Screen::Root: return 5;
    case Screen::Read:
    case Screen::DeleteList: return book_count_ + 1;
    case Screen::RecordMenu:
      return !audio_storage_ready_ ? 1 : recording_available_ ? 3 : 2;
    case Screen::Manage: return 3;
    case Screen::DeleteConfirm: return 2;  // Cancelar, Borrar
    case Screen::RecordingDeleteConfirm: return 2;
    case Screen::ReadEnd: return 2;        // Releer, Volver al menú
    case Screen::Settings: return 5;       // Brillo, Contraste, Wi-Fi, Velocidad de lectura, Atrás
    case Screen::Brightness:
      return (settings_.brightness_mode == settings::BrightnessMode::Manual ? 2 : 1) + 1;
    case Screen::Contrast:
    case Screen::Wifi:
    case Screen::SpeedStep: return 2;
    case Screen::Upload:
    case Screen::RecordingUpload:
    case Screen::About: return 1;
    case Screen::Recording: return 2;
    case Screen::Reading:
    case Screen::Message: return 0;
  }
  return 0;
}

bool Ui::has_back_item() const {
  switch (screen_) {
    case Screen::Read:
    case Screen::Manage:
    case Screen::DeleteList:
    case Screen::Upload:
    case Screen::Settings:
    case Screen::Brightness:
    case Screen::Contrast:
    case Screen::Wifi:
    case Screen::SpeedStep:
    case Screen::About:
    case Screen::RecordMenu:
      return true;
    default:
      return false;
  }
}

void Ui::enter(Screen screen) {
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
  if (screen_ == Screen::DeleteConfirm) enter(Screen::DeleteList);
  if (screen_ == Screen::Read || screen_ == Screen::DeleteList) enter(screen_);
}

void Ui::set_recording_available(bool available) {
  recording_available_ = available;
  if (screen_ == Screen::RecordMenu) enter(Screen::RecordMenu);
}

void Ui::set_audio_storage_ready(bool ready) {
  audio_storage_ready_ = ready;
  if (screen_ == Screen::RecordMenu) enter(Screen::RecordMenu);
}

Outcome Ui::handle(Event event) {
  if (screen_ == Screen::Upload) return on_upload_event(event);
  if (screen_ == Screen::Recording || screen_ == Screen::RecordingUpload) {
    return on_recording_event(event);
  }
  if (screen_ == Screen::Reading) return on_reading_event(event);
  if (event == Event::AudioAvailable) {
    set_recording_available(true);
    return {};
  }
  if (event == Event::AudioStorageUnavailable) {
    set_audio_storage_ready(false);
    return {};
  }
  if (event == Event::AudioSaved) {
    set_recording_available(true);
    enter(Screen::RecordMenu);
    show_message(Message::AudioSaved);
    return {};
  }
  if (event == Event::AudioDeleted) {
    set_recording_available(false);
    enter(Screen::RecordMenu);
    show_message(Message::AudioDeleted);
    return {};
  }
  if (event == Event::AudioTransferred) {
    set_recording_available(false);
    enter(Screen::RecordMenu);
    show_message(Message::AudioTransferred);
    return {Command::RecordingTransferred, 0};
  }
  if (event == Event::AudioStartFailed || event == Event::AudioBusy ||
      event == Event::AudioSaveFailed || event == Event::AudioNoSpace ||
      event == Event::AudioDeleteFailed || event == Event::AudioUploadStartFailed ||
      event == Event::AudioUploadTimedOut) {
    enter(Screen::RecordMenu);
    Message message = Message::AudioStartFailed;
    if (event == Event::AudioNoSpace) message = Message::AudioNoSpace;
    if (event == Event::AudioBusy) message = Message::AudioBusy;
    if (event == Event::AudioSaveFailed) message = Message::AudioSaveFailed;
    if (event == Event::AudioDeleteFailed) message = Message::AudioDeleteFailed;
    if (event == Event::AudioUploadStartFailed) message = Message::AudioUploadStartFailed;
    if (event == Event::AudioUploadTimedOut) message = Message::AudioUploadTimedOut;
    show_message(message);
    return {};
  }
  if (event == Event::UpRepeat || event == Event::DownRepeat) {
    if (!editing_) return {};
    const int direction = event == Event::UpRepeat ? +1 : -1;
    switch (screen_) {
      case Screen::Brightness:
        if (settings_.brightness_mode == settings::BrightnessMode::Manual && cursor() == 1) {
          return change(direction);
        }
        return {};
      case Screen::Contrast:
      case Screen::SpeedStep:
        if (cursor() != 0) return {};
        return change(direction);
      default:
        return {};
    }
  }
  if (screen_ == Screen::Message) {
    if (event == Event::Select || event == Event::Back) enter(return_to_);
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
    case Event::Select:
      enter(Screen::Manage);
      return {Command::StopUpload, 0};
    case Event::UploadStartFailed:
      enter(Screen::Manage);
      show_message(Message::ApStartFailed);
      return {};
    case Event::UploadSaved:
      enter(Screen::Manage);
      show_message(Message::UploadSaved);
      return {};
    case Event::UploadTimedOut:
      enter(Screen::Manage);
      show_message(Message::UploadTimedOut);
      return {};
    default:
      return {};
  }
}

Outcome Ui::on_recording_event(Event event) {
  switch (event) {
    case Event::RecordingStarted:
      recording_paused_ = false;
      return {};
    case Event::RecordingPaused:
      recording_paused_ = true;
      return {};
    case Event::RecordingResumed:
      recording_paused_ = false;
      return {};
    case Event::AudioSaved:
      recording_available_ = true;
      enter(Screen::RecordMenu);
      show_message(Message::AudioSaved);
      return {};
    case Event::AudioStartFailed:
      enter(Screen::RecordMenu);
      show_message(Message::AudioStartFailed);
      return {};
    case Event::AudioBusy:
      enter(Screen::RecordMenu);
      show_message(Message::AudioBusy);
      return {};
    case Event::AudioSaveFailed:
      enter(Screen::RecordMenu);
      show_message(Message::AudioSaveFailed);
      return {};
    case Event::AudioNoSpace:
      enter(Screen::RecordMenu);
      show_message(Message::AudioNoSpace);
      return {};
    case Event::AudioUploadStartFailed:
    case Event::AudioUploadTimedOut:
      enter(Screen::RecordMenu);
      show_message(event == Event::AudioUploadStartFailed ? Message::AudioUploadStartFailed
                                                          : Message::AudioUploadTimedOut);
      return {};
    case Event::Up:
    case Event::Down:
      if (screen_ == Screen::Recording) move(event);
      return {};
    case Event::AudioTransferred:
      recording_available_ = false;
      enter(Screen::RecordMenu);
      show_message(Message::AudioTransferred);
      return {Command::RecordingTransferred, 0};
    case Event::Back:
      if (screen_ == Screen::RecordingUpload) {
        enter(Screen::RecordMenu);
        return {Command::StopRecordingUpload, 0};
      }
      return {Command::StopRecording, 0};
    case Event::Select:
      if (screen_ == Screen::RecordingUpload) {
        enter(Screen::RecordMenu);
        return {Command::StopRecordingUpload, 0};
      }
      if (cursor() == 0) {
        return {recording_paused_ ? Command::ResumeRecording : Command::PauseRecording, 0};
      }
      return {Command::StopRecording, 0};
    default:
      return {};
  }
}

// While reading, the buttons belong to playback. The adapter decides what each one
// does, since UP/DOWN mean speed or seek depending on whether the reader is paused.
Outcome Ui::on_reading_event(Event event) {
  switch (event) {
    case Event::Up:
      if (!reading_paused_) {
        settings_.last_reading_wpm = settings::adjust_reading_wpm(
            settings_.last_reading_wpm, +1, settings_.reading_step_ppm);
      }
      return {Command::ReadingUp, 0};
    case Event::Down:
      if (!reading_paused_) {
        settings_.last_reading_wpm = settings::adjust_reading_wpm(
            settings_.last_reading_wpm, -1, settings_.reading_step_ppm);
      }
      return {Command::ReadingDown, 0};
    case Event::UpRepeat:
      if (reading_paused_) return {};
      settings_.last_reading_wpm = settings::adjust_reading_wpm(
          settings_.last_reading_wpm, +1, settings_.reading_step_ppm);
      return {Command::ReadingUp, 0};
    case Event::DownRepeat:
      if (reading_paused_) return {};
      settings_.last_reading_wpm = settings::adjust_reading_wpm(
          settings_.last_reading_wpm, -1, settings_.reading_step_ppm);
      return {Command::ReadingDown, 0};
    case Event::Select:
      reading_paused_ = !reading_paused_;
      return {Command::ReadingSelect, 0};
    case Event::Back:
      reading_paused_ = false;
      enter(Screen::Read);
      return {Command::CloseBook, 0};
    case Event::ReaderFinished:
      reading_paused_ = false;
      enter(Screen::ReadEnd);
      return {Command::SaveReadingSpeed, 0};
    case Event::BookOpenFailed:
      reading_paused_ = false;
      enter(Screen::Read);
      show_message(Message::BookOpenFailed);
      return {};
    default:
      return {};
  }
}

Outcome Ui::select() {
  if (has_back_item() && cursor() == item_count() - 1) return back();

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
    case Screen::RecordMenu:
      if (!recording_available_) {
        if (cursor() == 0) {
          enter(Screen::Recording);
          return {Command::StartRecording, 0};
        }
        return back();
      }
      if (cursor() == 0) {
        if (settings_.wifi == settings::WifiMode::Off) {
          show_message(Message::WifiOff);
          return {};
        }
        enter(Screen::RecordingUpload);
        return {Command::StartRecordingUpload, 0};
      }
      if (cursor() == 1) {
        enter(Screen::RecordingDeleteConfirm);
        return {};
      }
      return back();
    case Screen::RecordingDeleteConfirm:
      if (cursor() == 0) {
        enter(Screen::RecordMenu);
        return {};
      }
      return {Command::DeleteRecording, 0};
    case Screen::Read:
      if (book_count_ == 0) return {};
      reading_book_ = cursor();  // read before enter() resets the cursor
      reading_paused_ = false;
      enter(Screen::Reading);
      return {Command::OpenBook, reading_book_};
    case Screen::ReadEnd:
      if (cursor() == 0) {  // Releer: the adapter finds no saved position and starts at 0
        reading_paused_ = false;
        enter(Screen::Reading);
        return {Command::OpenBook, reading_book_};
      }
      enter(Screen::Read);
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
    case Screen::DeleteList:
      if (book_count_ == 0) return {};
      pending_book_ = cursor();
      enter(Screen::DeleteConfirm);
      return {};
    case Screen::DeleteConfirm:
      if (cursor() == 0) {  // Cancelar
        enter(Screen::DeleteList);
        return {};
      }
      enter(Screen::DeleteList);
      return {Command::DeleteBook, pending_book_};
    case Screen::Settings:
      if (cursor() == 0) enter(Screen::Brightness);
      if (cursor() == 1) enter(Screen::Contrast);
      if (cursor() == 2) enter(Screen::Wifi);
      if (cursor() == 3) enter(Screen::SpeedStep);
      return {};
    case Screen::Brightness:
    case Screen::Contrast:
    case Screen::Wifi:
    case Screen::SpeedStep:
      editing_ = !editing_;
      return {};
    default:
      return {};
  }
}

Outcome Ui::change(int direction) {
  switch (screen_) {
    case Screen::Brightness:
      if (cursor() == 0) {
        settings_.brightness_mode = settings::cycle_brightness(settings_.brightness_mode);
        const bool was_editing = editing_;
        enter(Screen::Brightness);  // the manual level row appears or disappears
        editing_ = was_editing;
        return {Command::SaveSettings, 0};
      }
      {
        const std::uint8_t next = settings::step_percent(settings_.brightness_percent, direction);
        if (next == settings_.brightness_percent) return {};
        settings_.brightness_percent = next;
      }
      return {Command::SaveSettings, 0};
    case Screen::Contrast: {
      const std::uint8_t next = settings::step_percent(settings_.contrast_percent, direction);
      if (next == settings_.contrast_percent) return {};
      settings_.contrast_percent = next;
      return {Command::SaveSettings, 0};
    }
    case Screen::Wifi:
      settings_.wifi = settings::cycle_wifi(settings_.wifi, direction);
      return {Command::SaveSettings, 0};
    case Screen::SpeedStep: {
      const std::uint16_t next = settings::adjust_reading_step(settings_.reading_step_ppm, direction);
      if (next == settings_.reading_step_ppm) return {};
      settings_.reading_step_ppm = next;
      settings_.last_reading_wpm = static_cast<std::uint16_t>(
          rsvp::normalize_wpm(settings_.last_reading_wpm, settings_.reading_step_ppm));
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
    case Screen::Read:
    case Screen::Manage:
    case Screen::Settings:
    case Screen::About:
    case Screen::RecordMenu:
      enter(Screen::Root);
      return {};
    case Screen::Recording:
      return {Command::StopRecording, 0};
    case Screen::RecordingUpload:
      enter(Screen::RecordMenu);
      return {Command::StopRecordingUpload, 0};
    case Screen::RecordingDeleteConfirm:
      enter(Screen::RecordMenu);
      return {};
    case Screen::DeleteList:
      enter(Screen::Manage);
      return {};
    case Screen::DeleteConfirm:
      enter(Screen::DeleteList);
      return {};
    case Screen::ReadEnd:
      enter(Screen::Read);
      return {Command::CloseBook, 0};
    case Screen::Brightness:
    case Screen::Contrast:
    case Screen::Wifi:
    case Screen::SpeedStep:
      enter(Screen::Settings);
      return {};
    case Screen::Message:
      enter(return_to_);
      return {};
    default:
      return {};
  }
}

}  // namespace leeloo::ui
