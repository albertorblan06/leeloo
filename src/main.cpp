#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>

#include "buttons.h"
#include "display_ui.h"
#include "inactivity.h"
#include "reader.h"
#include "settings_store.h"
#include "storage_books.h"
#include "ui.h"
#include "upload.h"
#include "upload_server.h"

// Device menu (docs/DEVICE-MENU.md) on three buttons. Hardware-bound glue: the
// menu logic lives in lib/ui, the upload protocol in lib/upload. Covered by
// docs/hardware-checklists/menu.md.

namespace {

using leeloo::buttons::Debouncer;
using leeloo::buttons::Edge;
using leeloo::buttons::Press;
using leeloo::settings::WifiMode;
using leeloo::ui::Command;
using leeloo::ui::Event;
using leeloo::ui::Outcome;
using leeloo::ui::Screen;
using leeloo::ui::Ui;
using leeloo::reader::State;

// Three tactile buttons to GND with the internal pull-up (docs/hardware-checklists/buttons-input.md).
constexpr std::size_t kButtonCount = 3;
constexpr std::uint8_t kButtonPins[kButtonCount] = {4, 5, 6};  // UP, DOWN, SELECT
constexpr std::size_t kEdgeQueueLength = 16;
constexpr std::size_t kUiQueueLength = 16;
constexpr std::int32_t kIdleWaitMs = 500;  // how often the inactivity timer is checked
constexpr std::uint32_t kWpmLabelMs = 2000;  // how long the WPM label stays after a speed change

enum Button : std::uint8_t { kUp = 0, kDown = 1, kSelect = 2 };
constexpr const char* kButtonNames[kButtonCount] = {"ARRIBA", "ABAJO", "SELECT"};

struct RawEdge {
  std::uint8_t button;
  bool high;
  std::uint32_t ms;
};

QueueHandle_t g_edge_queue = nullptr;
QueueHandle_t g_ui_queue = nullptr;  // carries leeloo::ui::Event from every source

// Buttons become menu events here, so the UI never sees hardware.
Event to_event(Button button, Press kind) {
  switch (button) {
    case kUp: return Event::Up;
    case kDown: return Event::Down;
    case kSelect: return kind == Press::Long ? Event::Back : Event::Select;
  }
  return Event::Select;
}

// Tasks only, never ISRs. A short wait keeps a burst of input from dropping
// a server event such as UploadSaved, which the menu depends on.
void post(Event event) { xQueueSend(g_ui_queue, &event, pdMS_TO_TICKS(100)); }

// --- Interrupts: only queue the raw edge, never debounce here -----------------

// One ISR serves all three buttons; the argument is the button index.
void IRAM_ATTR on_button_edge(void* arg) {
  const auto button = static_cast<std::uint8_t>(reinterpret_cast<std::uintptr_t>(arg));
  RawEdge edge{button, digitalRead(kButtonPins[button]) == HIGH, millis()};
  BaseType_t woken = pdFALSE;
  xQueueSendFromISR(g_edge_queue, &edge, &woken);
  if (woken == pdTRUE) portYIELD_FROM_ISR();
}

// --- Button task: debounce, classify, forward ---------------------------------

struct ButtonState {
  Debouncer debouncer;
  std::uint32_t pressed_at = 0;
};

// UP and DOWN act on the press, so scrolling is immediate. SELECT acts on the
// release, because its length decides between a short and a long press.
void handle_button_level(std::uint8_t button, bool high, std::uint32_t now, ButtonState* state) {
  const Edge edge = state->debouncer.update(high, now);
  if (edge == Edge::Pressed) {
    state->pressed_at = now;
    Serial.printf("%s\n", kButtonNames[button]);
    if (button != kSelect) post(to_event(static_cast<Button>(button), Press::Short));
  } else if (edge == Edge::Released && button == kSelect) {
    const Press kind = leeloo::buttons::classify(leeloo::buttons::elapsed_ms(state->pressed_at, now));
    post(to_event(kSelect, kind));
  }
}

// Blocks until an edge arrives. While a button waits for its debounce window, the
// queue wait times out after one window and the pending pins are sampled so the
// debouncer can confirm the level. Idle = no timer, no polling.
void button_task(void*) {
  ButtonState states[kButtonCount];
  bool settling = false;
  for (;;) {
    RawEdge edge;
    const TickType_t wait = settling ? pdMS_TO_TICKS(leeloo::buttons::kDebounceMs) : portMAX_DELAY;
    if (xQueueReceive(g_edge_queue, &edge, wait) == pdTRUE) {
      settling = true;
      handle_button_level(edge.button, edge.high, edge.ms, &states[edge.button]);
      continue;
    }
    settling = false;
    for (std::uint8_t b = 0; b < kButtonCount; ++b) {
      if (!states[b].debouncer.pending()) continue;
      handle_button_level(b, digitalRead(kButtonPins[b]) == HIGH, millis(), &states[b]);
      // The timeout can fire a tick before the window is over; keep sampling until
      // the change is confirmed or rejected, otherwise the press is lost.
      settling = settling || states[b].debouncer.pending();
    }
  }
}

// --- App state: Wi-Fi session bookkeeping and command execution ---------------

struct AppState {
  leeloo::upload::InactivityTimer timer{leeloo::upload::kUploadInactivityMs};
  bool auto_session = false;  // Automático: the session ends after inactivity
  WifiMode applied_wifi = WifiMode::Auto;
  // Automático stops the AP after a save, but not inside the upload request: the
  // 201 must reach the phone first, so the stop is deferred by a short grace time.
  std::uint32_t stop_at_ms = 0;
};

constexpr std::uint32_t kStopAfterSaveMs = 1500;

leeloo::settings::Settings g_boot_settings;
Ui* g_ui = nullptr;
AppState g_app;
bool g_redraw = true;

void set_display_contrast(const leeloo::settings::Settings& s) {
  leeloo::display::set_contrast(leeloo::settings::display_register(s.brightness_percent, s.contrast_percent));
}

// Stops the AP unless the On setting keeps it up while the device is awake.
void end_upload_session() {
  g_app.auto_session = false;
  g_app.timer.stop();
  g_app.stop_at_ms = 0;
  if (g_app.applied_wifi != WifiMode::On) leeloo::upload_server::stop();
}

// Keeps the menu's book count in step with the library. Only on change: the
// count is not reset on every event, which would move the list cursor back to 0.
void sync_books() {
  const std::size_t count = leeloo::storage::book_count();
  if (count != g_ui->book_count()) g_ui->set_book_count(count);
}

// --- Reading session: one book in PSRAM, one Reader over its text -------------

std::optional<leeloo::reader::Reader> g_reader;
leeloo::storage::BookText g_text;
// Title, not index: the library index can shift while the book is open (an upload may add a book).
char g_reading_title[64] = "";

void dispatch(Event event);

// Until this time the WPM label is shown; 0 means not shown.
std::uint32_t g_wpm_label_until = 0;

void close_book() {
  g_reader.reset();
  g_wpm_label_until = 0;
  leeloo::storage::free_text(&g_text);
}

// Stores the word the reader stopped on. A finished reader offers offset 0, so
// the next open starts at the beginning.
void save_progress() {
  if (!g_reader) return;
  if (!leeloo::storage::save_position(g_reading_title, g_reader->offset())) {
    Serial.println("Lectura: no se pudo guardar la posición");
  }
}

// Loads book `index` at its saved word. False when the text cannot be used.
// The library lock spans the lookup and the load, so the index names one book.
bool open_book(std::size_t index) {
  close_book();
  leeloo::storage::Guard guard;
  if (!leeloo::storage::title_at(index, g_reading_title, sizeof g_reading_title)) return false;
  if (!leeloo::storage::load_text(index, &g_text)) return false;
  const auto body = leeloo::reader::body_offset(g_text.data, g_text.size);
  if (!body) {
    close_book();
    return false;
  }
  std::size_t saved = 0;
  if (!leeloo::storage::load_position(g_reading_title, &saved)) saved = 0;
  g_reader.emplace(g_text.data + *body, g_text.size - *body, saved, millis(), leeloo::display::piece_count);
  return true;
}

// The last word has been held: the position resets to 0, then the book closes.
void finish_book() {
  save_progress();
  close_book();
  dispatch(Event::ReaderFinished);
}

// Advances playback when the current word's hold has elapsed.
void tick_reader() {
  if (g_reader && g_reader->tick(millis())) {
    if (g_reader->state() == State::Finished) finish_book();
    g_redraw = true;
  }
  // Hides the WPM label once its time is up.
  if (g_wpm_label_until != 0 && static_cast<std::int32_t>(millis() - g_wpm_label_until) >= 0) {
    g_wpm_label_until = 0;
    g_redraw = true;
  }
}

void execute(const Outcome& out) {
  switch (out.command) {
    case Command::None:
      return;
    case Command::OpenBook:
      if (!open_book(out.index)) {
        dispatch(Event::BookOpenFailed);
        return;
      }
      // An empty book has nothing to hold, so it ends at once.
      if (g_reader->state() == State::Finished) finish_book();
      return;
    case Command::CloseBook:
      save_progress();
      close_book();
      return;
    case Command::ReadingSelect:
      if (!g_reader) return;
      if (g_reader->state() == State::Reading) {
        g_reader->pause();
        save_progress();
      } else {
        g_reader->resume(millis());
      }
      return;
    case Command::ReadingUp:
      if (!g_reader) return;
      if (g_reader->state() == State::Paused) {
        g_reader->step_sentence(-1);
        save_progress();  // a seek is kept even if power is lost while paused
      } else {
        g_reader->faster();
        g_wpm_label_until = millis() + kWpmLabelMs;
      }
      return;
    case Command::ReadingDown:
      if (!g_reader) return;
      if (g_reader->state() == State::Paused) {
        g_reader->step_sentence(+1);
        save_progress();  // a seek is kept even if power is lost while paused
      } else {
        g_reader->slower();
        g_wpm_label_until = millis() + kWpmLabelMs;
      }
      return;
    case Command::StartUpload: {
      g_app.stop_at_ms = 0;  // a deferred stop from an earlier save must not end this session
      char pin[leeloo::upload::kPinDigits + 1];
      if (!leeloo::upload_server::running() && !leeloo::upload_server::start(pin)) {
        Serial.println("Wi-Fi: no se pudo iniciar");
        dispatch(Event::UploadStartFailed);
        return;
      }
      g_app.auto_session = g_app.applied_wifi == WifiMode::Auto;
      if (g_app.auto_session) {
        // An AP that was already up may carry an old activity time; restart the window now.
        leeloo::upload_server::mark_activity();
        g_app.timer.start(millis());
      }
      return;
    }
    case Command::StopUpload:
      end_upload_session();
      return;
    case Command::DeleteBook:
      if (!leeloo::storage::remove_at(out.index)) dispatch(Event::DeleteFailed);
      sync_books();
      return;
    case Command::SaveSettings: {
      const auto& s = g_ui->settings();
      if (!leeloo::settings_store::save(s)) Serial.println("Ajustes: no se pudo guardar");
      set_display_contrast(s);
      // Wi-Fi changes apply immediately; Automático waits for Biblioteca > Subir. Leaving On
      // stops an AP that no session owns, unless the upload screen is open on it.
      if (s.wifi == WifiMode::On && !leeloo::upload_server::running()) {
        char pin[leeloo::upload::kPinDigits + 1];
        leeloo::upload_server::start(pin);
      }
      const bool owned_by_upload = g_ui->screen() == Screen::Upload;
      if (s.wifi != WifiMode::On && leeloo::upload_server::running() && !owned_by_upload) {
        leeloo::upload_server::stop();
      }
      g_app.applied_wifi = s.wifi;
      return;
    }
  }
}

// Runs one menu event: the state machine decides, then the adapter carries out the command.
void dispatch(Event event) {
  if (event == Event::UploadTimedOut) end_upload_session();
  // Only a save made from the upload screen schedules the stop; otherwise the session is already over.
  if (event == Event::UploadSaved && g_app.applied_wifi != WifiMode::On && g_ui->screen() == Screen::Upload) {
    g_app.stop_at_ms = millis() + kStopAfterSaveMs;
  }
  const Outcome out = g_ui->handle(event);
  execute(out);
  // Always refresh after a save, even when the count is unchanged: a replace keeps
  // the count but changes what the indexes point at.
  if (event == Event::UploadSaved) g_ui->set_book_count(leeloo::storage::book_count());
  g_redraw = true;
}

// Auto sessions end after the inactivity limit; the phone's requests keep them alive.
void check_inactivity() {
  if (g_app.stop_at_ms != 0 && static_cast<std::int32_t>(millis() - g_app.stop_at_ms) >= 0) {
    end_upload_session();
    return;
  }
  if (!g_app.auto_session || !leeloo::upload_server::running()) return;
  g_app.timer.touch(leeloo::upload_server::last_activity_ms());
  if (g_app.timer.expired(millis())) {
    Serial.println("Wi-Fi: sesión cerrada por inactividad");
    dispatch(Event::UploadTimedOut);
  }
}

void on_book_saved() { post(Event::UploadSaved); }

leeloo::display::Status display_status() {
  leeloo::display::Status status;
  status.storage_ok = leeloo::storage::mounted();
  status.free_bytes = leeloo::storage::free_bytes();
  status.ap_running = leeloo::upload_server::running();
  status.pin = leeloo::upload_server::current_pin();
  status.reader = g_reader ? &*g_reader : nullptr;
  status.show_wpm = g_wpm_label_until != 0;
  return status;
}

// Wakes at the next piece or word change while reading, at the WPM label's
// expiry, and otherwise at the idle rate for the inactivity timer.
TickType_t next_wait_ticks() {
  const std::uint32_t now = millis();
  std::int32_t wait_ms = kIdleWaitMs;
  if (g_reader && g_reader->state() == State::Reading) {
    wait_ms = std::min(wait_ms, static_cast<std::int32_t>(g_reader->wake_ms() - now));
  }
  if (g_wpm_label_until != 0) {
    wait_ms = std::min(wait_ms, static_cast<std::int32_t>(g_wpm_label_until - now));
  }
  return wait_ms <= 0 ? 0 : pdMS_TO_TICKS(static_cast<std::uint32_t>(wait_ms));
}

}  // namespace

void setup() {
  Serial.begin(115200);
  if (!leeloo::storage::mount()) Serial.println("LittleFS no montó: libros no disponibles.");
  g_boot_settings = leeloo::settings_store::load();

  if (!leeloo::display::init()) Serial.println("No OLED at 0x3C/0x3D: input still reports on serial.");
  set_display_contrast(g_boot_settings);

  g_edge_queue = xQueueCreate(kEdgeQueueLength, sizeof(RawEdge));
  g_ui_queue = xQueueCreate(kUiQueueLength, sizeof(Event));
  if (g_edge_queue == nullptr || g_ui_queue == nullptr) {
    Serial.println("Queue allocation failed; halting.");
    for (;;) delay(1000);
  }

  for (const std::uint8_t pin : kButtonPins) pinMode(pin, INPUT_PULLUP);
  xTaskCreate(button_task, "buttons", 4096, nullptr, 2, nullptr);
  for (std::uint8_t b = 0; b < kButtonCount; ++b) {
    attachInterruptArg(digitalPinToInterrupt(kButtonPins[b]), on_button_edge,
                       reinterpret_cast<void*>(static_cast<std::uintptr_t>(b)), CHANGE);
  }

  leeloo::upload_server::set_on_saved(on_book_saved);
  g_app.applied_wifi = g_boot_settings.wifi;
  if (g_boot_settings.wifi == WifiMode::On) {
    char pin[leeloo::upload::kPinDigits + 1];
    if (!leeloo::upload_server::start(pin)) Serial.println("Wi-Fi: no se pudo iniciar al arrancar");
  }
}

void loop() {
  static Ui ui(g_boot_settings);
  if (g_ui == nullptr) {
    g_ui = &ui;
    sync_books();  // the library may already hold books from an earlier session
  }

  if (g_redraw) {
    leeloo::display::render(ui, display_status());
    g_redraw = false;
  }

  Event event;
  if (xQueueReceive(g_ui_queue, &event, next_wait_ticks()) == pdTRUE) {
    dispatch(event);
    leeloo::display::render(ui, display_status());
    g_redraw = false;
  }
  tick_reader();
  check_inactivity();
}
