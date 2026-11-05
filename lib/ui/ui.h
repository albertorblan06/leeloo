#pragma once

#include <cstddef>
#include <cstdint>

#include "menu.h"
#include "settings.h"

// On-device menu state machine (docs/DEVICE-MENU.md). Pure logic: it receives
// events and returns one command for the adapter in src/ to carry out. It never
// touches the display, storage or Wi-Fi itself.
namespace leeloo::ui {

enum class Screen : std::uint8_t {
  Root,
  Read,
  Manage,
  Upload,
  DeleteList,
  DeleteConfirm,
  Settings,
  Brightness,
  Contrast,
  Wifi,
  About,
  Message,
  Reading,   // RSVP playback of the book chosen in Leer
  ReadEnd,   // end of book: Releer or Volver al menú
};

enum class Event : std::uint8_t {
  Up,
  Down,
  Left,
  Right,
  Select,          // short press: open, confirm, or change
  Back,            // long SELECT press: go to the parent screen
  UploadStarted,   // adapter: AP and HTTP server are running
  UploadStartFailed,
  UploadSaved,     // adapter: a book was stored
  UploadTimedOut,  // adapter: no upload activity for the inactivity limit
  DeleteFailed,    // adapter: the file could not be removed
  ReaderFinished,  // adapter: the last word of the book has been held
  BookOpenFailed,  // adapter: the book could not be loaded or has no text start
};

enum class Command : std::uint8_t {
  None,
  StartUpload,
  StopUpload,
  DeleteBook,      // index: book to remove
  OpenBook,        // index: book to play from its saved position (or the start)
  CloseBook,       // adapter: stop playback and save the position (or clear it at the end)
  ReadingSelect,   // adapter: pause, or resume when paused
  ReadingUp,       // adapter: faster while reading, previous word while paused
  ReadingDown,     // adapter: slower while reading, next word while paused
  SaveSettings,    // adapter: persist settings() and apply them
};

enum class Message : std::uint8_t {
  None,
  WifiOff,
  ApStartFailed,
  UploadSaved,
  UploadTimedOut,
  DeleteFailed,
  BookOpenFailed,
};

struct Outcome {
  Command command = Command::None;
  std::size_t index = 0;
};

class Ui {
 public:
  explicit Ui(const settings::Settings& initial) : settings_(initial) {}

  Outcome handle(Event event);

  // The adapter reports the stored book count after any change to the library.
  void set_book_count(std::size_t count);

  Screen screen() const { return screen_; }
  std::size_t cursor() const { return cursor_.index(); }
  Message message() const { return message_; }
  std::size_t book_count() const { return book_count_; }
  std::size_t pending_book() const { return pending_book_; }
  std::size_t reading_book() const { return reading_book_; }

  // True while a setting's value is being adjusted: SELECT toggles it and UP/DOWN
  // then change the value instead of moving the cursor (three-button input).
  bool editing() const { return editing_; }
  const settings::Settings& settings() const { return settings_; }

  // No ambient light sensor is wired yet, so Auto brightness cannot read it and
  // the manual level applies (docs/DEVICE-MENU.md §2 Brillo).
  bool sensor_unavailable() const {
    return settings_.brightness_mode == settings::BrightnessMode::Auto;
  }

 private:
  std::size_t item_count() const;
  void enter(Screen screen);
  void show_message(Message message);
  void move(Event event);
  Outcome select();
  Outcome back();
  Outcome change(int direction);
  Outcome on_upload_event(Event event);
  Outcome on_reading_event(Event event);

  Screen screen_ = Screen::Root;
  Screen return_to_ = Screen::Root;
  Message message_ = Message::None;
  menu::Menu cursor_{4};
  std::size_t book_count_ = 0;
  std::size_t pending_book_ = 0;  // book chosen for deletion
  std::size_t reading_book_ = 0;  // book being read, reopened by Releer
  bool editing_ = false;
  settings::Settings settings_;
};

}  // namespace leeloo::ui
