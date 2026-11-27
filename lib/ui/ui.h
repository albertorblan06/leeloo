#pragma once

#include <cstddef>
#include <cstdint>

#include "menu.h"
#include "settings.h"

// On-device menu state machine (docs/DEVICE-MENU.md). Pure logic: it receives
// events and returns one command for the adapter in src/ to carry out. It never
// touches the display, storage or Wi-Fi itself.
namespace leeloo::ui {

// Leer, Biblioteca, Grabadora, Ajustes, Info.
constexpr std::size_t kRootItems = 5;

enum class Screen : std::uint8_t {
  Root,
  Read,
  Manage,
  Upload,
  DeleteList,
  DeleteConfirm,
  Settings,
  Brightness,
  Wifi,
  SpeedStep,
  About,
  Message,
  Reading,   // RSVP playback of the book chosen in Leer
  ReadEnd,   // end of book: Releer or Volver al menú
  RecordMenu,              // Nueva grabación, or Enviar al móvil / Borrar grabación when a WAV is pending
  Recording,               // capture in progress: Pausar/Reanudar, Detener
  RecordingUpload,         // the Wi-Fi session that hands the WAV to the phone
  RecordingDeleteConfirm,  // Cancelar, Borrar
  PowerConfirm,            // ¿Apagar?: No (focused), Sí, apagar
  PoweringOff,             // Apagando: not interactive, the display task then puts the device to sleep
};

enum class Event : std::uint8_t {
  Up,
  Down,
  UpRepeat,        // a held UP: acts only while reading or while editing a number
  DownRepeat,
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
  // Audio events come from the recorder, the HTTP task and the net service. They can
  // arrive on any screen, so the Ui handles them before looking at the screen.
  AudioAvailable,          // a finished WAV is stored (boot, or after a save)
  AudioStorageUnavailable, // storage is not mounted: recording is not possible
  AudioSaved,              // the recorder finished and stored a WAV
  AudioDeleted,
  AudioDeleteFailed,
  AudioTransferred,        // the phone confirmed it received the whole WAV
  AudioStartFailed,        // the recorder could not start (I2S, file, a WAV already pending)
  AudioBusy,               // another storage operation is running
  AudioNoSpace,
  AudioSaveFailed,         // writing or finalizing failed, or nothing was recorded
  AudioUploadStartFailed,
  AudioUploadTimedOut,
  RecordingEnded,          // a Stop arrived while the recorder was not recording
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
  ReadingUpRepeat,    // adapter: held UP; faster while reading, ignored while paused
  ReadingDownRepeat,  // adapter: held DOWN; slower while reading, ignored while paused
  SaveSettings,    // adapter: persist settings() and apply them
  StartRecording,
  ToggleRecordingPause,  // the recorder pauses or resumes, whichever applies
  StopRecording,         // stop and save; the recorder answers with AudioSaved or an error
  DeleteRecording,
  StartRecordingUpload,
  StopRecordingUpload,
  RecordingTransferred,  // the WAV is gone from the device: the net service may stop the AP
};

enum class Message : std::uint8_t {
  None,
  WifiOff,
  ApStartFailed,
  UploadSaved,
  UploadTimedOut,
  DeleteFailed,
  BookOpenFailed,
  AudioStartFailed,
  AudioBusy,
  AudioNoSpace,
  AudioSaveFailed,
  AudioSaved,
  AudioDeleted,
  AudioDeleteFailed,
  AudioTransferred,
  AudioUploadStartFailed,
  AudioUploadTimedOut,
};

struct Outcome {
  Command command = Command::None;
  std::size_t index = 0;
};

class Ui {
 public:
  explicit Ui(const settings::Settings& initial) : settings_(settings::sanitize(initial)) {}

  Outcome handle(Event event);

  // The adapter reports the stored book count after any change to the library.
  void set_book_count(std::size_t count);

  Screen screen() const { return screen_; }
  std::size_t cursor() const { return cursor_.index(); }
  Message message() const { return message_; }
  std::size_t book_count() const { return book_count_; }
  std::size_t pending_book() const { return pending_book_; }
  std::size_t reading_book() const { return reading_book_; }

  // True when the cursor is on the fixed `< Atrás` row of the screen.
  bool back_focused() const { return has_back_item() && cursor() + 1 == item_count(); }

  // True when the last screen change went back to a parent, so the display slides
  // the other way. Changes only when the screen does.
  bool back_navigation() const { return back_navigation_; }

  // Whether a finished WAV is stored, and whether the recorder can use the storage.
  bool recording_available() const { return recording_available_; }
  bool audio_storage_ready() const { return audio_storage_ready_; }

  // True while a setting's value is being adjusted: SELECT toggles it and UP/DOWN
  // then change the value instead of moving the cursor (three-button input).
  bool editing() const { return editing_; }
  const settings::Settings& settings() const { return settings_; }

 private:
  std::size_t item_count() const;
  bool has_back_item() const;
  void enter(Screen screen);
  void enter_back(Screen screen);  // enter() for a move to the parent screen
  void show_message(Message message);
  void move(Event event);
  Outcome select();
  Outcome back();
  Outcome change(int direction);
  Outcome on_upload_event(Event event);
  Outcome on_reading_event(Event event);
  Outcome on_repeat(Event event);
  Outcome on_audio_event(Event event);
  Outcome on_recording_event(Event event);
  Outcome on_recording_upload_event(Event event);
  void set_recording_available(bool available);
  void set_audio_storage_ready(bool ready);

  Screen screen_ = Screen::Root;
  Screen return_to_ = Screen::Root;
  Message message_ = Message::None;
  menu::Menu cursor_{kRootItems};  // starts on Root, so it must match item_count() there
  std::size_t book_count_ = 0;
  std::size_t pending_book_ = 0;  // book chosen for deletion
  std::size_t reading_book_ = 0;  // book being read, reopened by Releer
  bool editing_ = false;
  bool recording_available_ = false;
  bool audio_storage_ready_ = true;
  bool back_navigation_ = false;
  bool entering_back_ = false;
  settings::Settings settings_;
};

}  // namespace leeloo::ui
