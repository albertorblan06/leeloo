#include "display_ui.h"

#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>

#include <cstdio>
#include <cstring>
#include <utility>

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include "boot_anim.h"
#include "reading_frame.h"
#include "recording.h"
#include "rsvp_view.h"
#include "storage_books.h"
#include "text.h"
#include "ui_text.h"
#include "upload_server.h"

namespace leeloo::display {
namespace {

using leeloo::settings::WifiMode;
using leeloo::ui::Screen;

constexpr int kSdaPin = 8;
constexpr int kSclPin = 10;
constexpr const char* kFirmwareVersion = "0.1.0-dev";

// Grid from DESIGN.md §3.1: title y0-9, rule y11, four rows at 14/26/38/50. The
// root menu has no title, so it spreads its rows over the full height.
constexpr int kRowY[4] = {14, 26, 38, 50};
constexpr int kRootRowY[4] = {7, 22, 37, 52};
constexpr int kRowWidth = 128;
constexpr std::size_t kVisibleRows = 4;
constexpr std::size_t kListRows = kVisibleRows - 1;  // the fourth row is always `< Atrás`
constexpr int kBackSlot = 3;

// A full-screen slide between two frames (DESIGN.md §3.4): 8 steps of 16 px.
constexpr std::size_t kFrameWidth = 128;
constexpr std::size_t kFramePages = 8;
constexpr std::size_t kFrameBytes = kFrameWidth * kFramePages;
constexpr std::size_t kSlideSteps = 8;
constexpr std::size_t kTitleChars = 20;  // 6 px per character, 120 of the 124 px usable

// U8g2 wants the 8-bit form of the address; the SH1106 is the likely controller.
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
bool g_ready = false;

// u8g2 keeps one current font and one frame buffer. The display task draws with
// them and the RSVP task measures words with them (piece_count), so both hold this
// lock. The I2C transfer in render() runs outside it: it touches neither, which
// keeps the RSVP task from waiting for a ~25 ms screen dump. A mutex, so a waiting
// high-priority task boosts the holder (priority inheritance).
SemaphoreHandle_t g_u8g2_lock = nullptr;

class U8g2Lock {
 public:
  U8g2Lock() { xSemaphoreTake(g_u8g2_lock, portMAX_DELAY); }
  ~U8g2Lock() { xSemaphoreGive(g_u8g2_lock); }
  U8g2Lock(const U8g2Lock&) = delete;
  U8g2Lock& operator=(const U8g2Lock&) = delete;
};

// First visible list row, kept between frames so the list scrolls smoothly.
std::size_t g_list_start = 0;
Screen g_list_screen = Screen::Root;

// The frame on the panel and the one being drawn, kept to slide between them.
std::uint8_t g_previous_frame[kFrameBytes];
std::uint8_t g_next_frame[kFrameBytes];
Screen g_previous_screen = Screen::Root;
bool g_have_previous_frame = false;

void draw_title(const char* text) {
  u8g2.drawUTF8(0, 0, text);
  u8g2.drawHLine(0, 11, 128);
}

void draw_line(int slot, const char* text) { u8g2.drawUTF8(3, kRowY[slot], text); }

// One menu row at pixel row `y`: label left, optional value right, inverted when focused.
void draw_row_at(int y, const char* label, const char* value, bool focused) {
  if (focused) {
    u8g2.setDrawColor(1);
    u8g2.drawBox(0, y - 1, kRowWidth, 12);
    u8g2.setDrawColor(0);
  }
  u8g2.drawUTF8(3, y, label);
  if (value != nullptr) {
    u8g2.drawUTF8(kRowWidth - 3 - u8g2.getUTF8Width(value), y, value);
  }
  u8g2.setDrawColor(1);
}

void draw_row(int slot, const char* label, const char* value, bool focused) {
  draw_row_at(kRowY[slot], label, value, focused);
}

// The fixed bottom row of every screen with a parent. Text only, so it needs no glyph.
void draw_back_row(bool focused) { draw_row(kBackSlot, "< Atrás", nullptr, focused); }

// Labels for the list rows that are on screen, scrolled to keep the cursor visible
// in the three rows above the fixed `< Atrás` row. The cursor sits on that row
// when it equals `count`.
template <typename LabelFn>
void draw_list(const char* title, std::size_t count, std::size_t cursor, const char* empty, LabelFn label, const char* value) {
  draw_title(title);
  if (count == 0) {
    draw_line(0, empty);
  } else {
    const std::size_t list_cursor = cursor < count ? cursor : count - 1;
    g_list_start = leeloo::text::scroll_start(g_list_start, list_cursor, count, kListRows);
    for (std::size_t slot = 0; slot < kListRows && g_list_start + slot < count; ++slot) {
      const std::size_t i = g_list_start + slot;
      draw_row(static_cast<int>(slot), label(i), value, i == cursor);
    }
  }
  draw_back_row(cursor >= count);
}

const char* wifi_label(WifiMode mode) {
  switch (mode) {
    case WifiMode::Off: return "Apagado";
    case WifiMode::Auto: return "Automático";
    case WifiMode::On: return "Encendido";
  }
  return "";
}

// Book title for the list, cut to the row width. Empty when the index is gone.
void book_label(std::size_t index, char* out, std::size_t cap) {
  char raw[64];
  if (!leeloo::storage::title_at(index, raw, sizeof raw)) {
    out[0] = '\0';
    return;
  }
  const std::size_t len = std::strlen(raw);
  if (leeloo::text::truncate_chars(raw, len, kTitleChars, "...", out, cap) == 0) out[0] = '\0';
}

// The WPM and PAUSA labels share the top-left corner: a 6x10 line that stays
// clear of the focal marks at x = 64.
void draw_reading(const leeloo::app::ReadingFrame& frame) {
  if (!frame.active) return;
  const std::size_t length = std::strlen(frame.word);
  if (length > 0) leeloo::view::draw_piece(u8g2, frame.word, length, frame.piece);

  char label[16] = "";
  if (frame.state == leeloo::reader::State::Paused) {
    std::snprintf(label, sizeof label, "PAUSA");
  } else if (frame.show_wpm) {
    std::snprintf(label, sizeof label, "%lu PPM", static_cast<unsigned long>(frame.wpm));
  }
  if (label[0] != '\0') u8g2.drawUTF8(0, 0, label);
}

void draw_read_end(const leeloo::ui::Ui& ui) {
  draw_title("LIBRO TERMINADO");
  draw_row(0, "Releer", nullptr, ui.cursor() == 0);
  draw_row(1, "Volver al menú", nullptr, ui.cursor() == 1);
}

void draw_read(const Status& status, const leeloo::ui::Ui& ui) {
  if (!status.storage_ok) {
    draw_title("LEER");
    draw_line(0, "Almacenamiento");
    draw_line(1, "no disponible");
    draw_back_row(ui.back_focused());
    return;
  }
  char label[80];
  draw_list(
      "LEER", ui.book_count(), ui.cursor(), "Sin libros",
      [&label](std::size_t i) {
        book_label(i, label, sizeof label);
        return static_cast<const char*>(label);
      },
      nullptr);
  if (ui.book_count() == 0) {
    draw_line(1, "Sube uno desde");
    draw_line(2, "Biblioteca.");
  }
}

void draw_delete_list(const leeloo::ui::Ui& ui) {
  char label[80];
  draw_list(
      "BORRAR LIBRO", ui.book_count(), ui.cursor(), "Sin libros",
      [&label](std::size_t i) {
        book_label(i, label, sizeof label);
        return static_cast<const char*>(label);
      },
      nullptr);
}

void draw_starting() { draw_line(0, "Iniciando..."); }

void draw_upload(const Status& status, const char* title = "AÑADIR LIBRO") {
  draw_title(title);
  draw_back_row(true);  // the only item, so always focused
  if (!status.ap_running) {
    draw_starting();
    return;
  }
  char network_line[32];
  std::snprintf(network_line, sizeof network_line, "Red: %s", leeloo::upload_server::kSsid);
  char pin_line[24];
  std::snprintf(pin_line, sizeof pin_line, "PIN: %s", status.pin);
  char address_line[32];
  std::snprintf(address_line, sizeof address_line, "Abre %s", leeloo::upload_server::kAddress);
  draw_line(0, network_line);
  draw_line(1, pin_line);
  draw_line(2, address_line);
}

void draw_message(const leeloo::ui::Ui& ui) {
  const leeloo::ui::MessageText text = leeloo::ui::message_text(ui.message());
  draw_line(0, text.line1);
  draw_line(1, text.line2);
  draw_line(3, "Pulsa para volver");
}

void draw_about(const Status& status, const leeloo::ui::Ui&) {
  draw_title("INFO");
  draw_row(0, "Firmware", kFirmwareVersion, false);
  char free_kb[16];
  std::snprintf(free_kb, sizeof free_kb, "%u KB", static_cast<unsigned>(status.free_bytes / 1024));
  draw_row(1, "Libre", free_kb, false);
  draw_row(2, "Apagar", "mantén SELECT", false);  // how to switch off, where people look for it
  draw_back_row(true);  // the only selectable row
}

// "< v >" shows a value that can be edited; "[ v ]" marks the one being edited now.
void format_value(char* out, std::size_t cap, const char* text, bool editing) {
  std::snprintf(out, cap, editing ? "[ %s ]" : "< %s >", text);
}

void draw_percent_row(int slot, const char* label, std::uint8_t percent, bool focused, bool editing) {
  char text[8];
  std::snprintf(text, sizeof text, "%u%%", static_cast<unsigned>(percent));
  char value[16];
  format_value(value, sizeof value, text, editing);
  draw_row(slot, label, value, focused);
}

void draw_brightness(const leeloo::ui::Ui& ui) {
  draw_title("BRILLO");
  draw_percent_row(0, "Nivel", ui.settings().brightness_percent, ui.cursor() == 0, ui.editing());
  draw_back_row(ui.back_focused());
}

void draw_wifi_setting(const leeloo::ui::Ui& ui) {
  draw_title("WI-FI");
  char value[32];
  format_value(value, sizeof value, wifi_label(ui.settings().wifi), ui.editing());
  draw_row(0, "Modo", value, ui.cursor() == 0);
  const leeloo::ui::MessageText help = leeloo::ui::wifi_help(ui.settings().wifi);
  draw_line(1, help.line1);
  draw_line(2, help.line2);
  draw_back_row(ui.back_focused());
}

// Five items in four rows: the window scrolls so the focused row stays visible.
void draw_root(const leeloo::ui::Ui& ui) {
  static const char* const kItems[] = {"Leer", "Biblioteca", "Grabadora", "Ajustes", "Info"};
  constexpr std::size_t kCount = sizeof kItems / sizeof kItems[0];
  g_list_start = leeloo::text::scroll_start(g_list_start, ui.cursor(), kCount, kVisibleRows);
  for (std::size_t slot = 0; slot < kVisibleRows && g_list_start + slot < kCount; ++slot) {
    const std::size_t i = g_list_start + slot;
    draw_row_at(kRootRowY[slot], kItems[i], ">", i == ui.cursor());
  }
}

// mm:ss; minutes wrap at 100, far beyond the 2-minute limit.
void format_clock(char* out, std::size_t cap, std::uint32_t seconds) {
  std::snprintf(out, cap, "%02u:%02u", static_cast<unsigned>(seconds / 60 % 100),
                static_cast<unsigned>(seconds % 60));
}

void draw_record_menu(const Status& status, const leeloo::ui::Ui& ui) {
  draw_title("GRABADORA");
  if (!ui.audio_storage_ready()) {
    draw_line(0, "Almacenamiento");
    draw_line(1, "no disponible");
  } else if (!ui.recording_available()) {
    draw_row(0, "Nueva grabación", ">", ui.cursor() == 0);
    // How long the next recording can be, from the space left right now.
    const std::uint32_t seconds = leeloo::recording::plan_format(status.free_bytes).max_seconds;
    if (seconds == 0) {
      draw_line(1, "Sin espacio. Libera");
      draw_line(2, "espacio para grabar.");
    } else {
      char clock[8];
      format_clock(clock, sizeof clock, seconds);
      char line[24];
      std::snprintf(line, sizeof line, "Disponible: %s", clock);
      draw_line(1, line);
    }
  } else {
    draw_row(0, "Enviar al móvil", ">", ui.cursor() == 0);
    draw_row(1, "Borrar grabación", ">", ui.cursor() == 1);
  }
  draw_back_row(ui.back_focused());
}

void draw_recording(const Status& status, const leeloo::ui::Ui& ui) {
  const leeloo::app::RecordingFrame& rec = status.recording;
  draw_title(rec.paused ? "PAUSA" : "GRABANDO");
  char elapsed[8];
  char limit[8];
  format_clock(elapsed, sizeof elapsed, rec.elapsed_seconds);
  format_clock(limit, sizeof limit, rec.max_seconds);
  char line[24];
  std::snprintf(line, sizeof line, "%s / %s", elapsed, limit);
  draw_line(0, line);
  draw_row(1, rec.paused ? "Reanudar" : "Pausar", nullptr, ui.cursor() == 0);
  draw_row(2, "Detener", nullptr, ui.cursor() == 1);
}

void draw_power_confirm(const leeloo::ui::Ui& ui) {
  draw_title("APAGAR");
  draw_line(0, "¿Apagar Leeloo?");
  draw_row(2, "No", nullptr, ui.cursor() == 0);
  draw_row(3, "Sí, apagar", nullptr, ui.cursor() == 1);
}

// Centered, so the last thing on the panel before it sleeps is not a menu.
void draw_powering_off() {
  const char* text = "Apagando...";
  u8g2.drawUTF8((128 - u8g2.getUTF8Width(text)) / 2, 28, text);
}

void draw_recording_delete_confirm(const leeloo::ui::Ui& ui) {
  draw_title("BORRAR GRABACIÓN");
  draw_line(0, "¿Borrar la grabación?");
  draw_row(2, "Cancelar", nullptr, ui.cursor() == 0);
  draw_row(3, "Borrar", nullptr, ui.cursor() == 1);
}

void draw_manage(const leeloo::ui::Ui& ui) {
  draw_title("BIBLIOTECA");
  draw_row(0, "Añadir libro", ">", ui.cursor() == 0);
  draw_row(1, "Borrar libro", ">", ui.cursor() == 1);
  draw_back_row(ui.back_focused());
}

void draw_settings(const leeloo::ui::Ui& ui) {
  static const char* const kItems[] = {"Brillo", "Salto por pulsación", "Wi-Fi"};
  constexpr std::size_t kCount = sizeof kItems / sizeof kItems[0];
  draw_title("AJUSTES");
  for (std::size_t i = 0; i < kCount; ++i) {  // three rows plus Atrás fit without scrolling
    draw_row(static_cast<int>(i), kItems[i], ">", i == ui.cursor());
  }
  draw_back_row(ui.back_focused());
}

void draw_speed_step(const leeloo::ui::Ui& ui) {
  draw_title("SALTO POR PULSACIÓN");
  char text[16];
  std::snprintf(text, sizeof text, "%u PPM", static_cast<unsigned>(ui.settings().reading_step_ppm));
  char value[24];
  format_value(value, sizeof value, text, ui.editing());
  draw_row(0, "Salto", value, ui.cursor() == 0);
  draw_line(1, "Cambia la velocidad");
  draw_line(2, "en cada pulsación.");
  draw_back_row(ui.back_focused());
}

void draw_delete_confirm(const leeloo::ui::Ui& ui) {
  draw_title("BORRAR LIBRO");
  char label[80];
  book_label(ui.pending_book(), label, sizeof label);  // the user sees which book is at stake
  draw_line(0, "¿Borrar este libro?");
  draw_line(1, label);
  draw_row(2, "Cancelar", nullptr, ui.cursor() == 0);
  draw_row(3, "Borrar", nullptr, ui.cursor() == 1);
}

}  // namespace

bool init() {
  if (g_u8g2_lock == nullptr) g_u8g2_lock = xSemaphoreCreateMutex();
  if (g_u8g2_lock == nullptr) return false;
  Wire.begin(kSdaPin, kSclPin);
  for (std::uint8_t address : {std::uint8_t{0x3C}, std::uint8_t{0x3D}}) {
    Wire.beginTransmission(address);
    if (Wire.endTransmission() != 0) continue;
    u8g2.setI2CAddress(address << 1);
    u8g2.begin();
    u8g2.setBusClock(400000);
    u8g2.enableUTF8Print();
    g_ready = true;
    return true;
  }
  return false;
}

bool ready() { return g_ready; }

// One frame of the boot logo: Loudop's dot with a fixed guide above, a "hand" that turns from
// the top round to the bottom, and the word typed out to the right. The mark is centred at x 36
// and the word starts at x 58, so the pair is centred on the 128 px panel.
void draw_boot_frame(const leeloo::boot_anim::Frame& frame) {
  using namespace leeloo::boot_anim;
  constexpr int kCx = 36;
  constexpr int kCy = 32;
  constexpr int kWordX = 58;
  u8g2.drawDisc(kCx, kCy, (kDotRadius * frame.dot_permille + 500) / 1000);
  u8g2.drawBox(kCx - 1, kCy - kHandOuter, 2, kHandOuter - kHandInner + 1);  // the fixed guide

  const Offset from = polar(frame.hand_degrees, kHandInner);
  const Offset to = polar(frame.hand_degrees, kHandOuter);
  // Two pixels thick like the guide: a second line beside the first, across the hand.
  const bool mostly_vertical = (to.x - from.x) * (to.x - from.x) <= (to.y - from.y) * (to.y - from.y);
  const int side_x = mostly_vertical ? -1 : 0;
  const int side_y = mostly_vertical ? 0 : -1;
  u8g2.drawLine(kCx + from.x, kCy + from.y, kCx + to.x, kCy + to.y);
  u8g2.drawLine(kCx + from.x + side_x, kCy + from.y + side_y, kCx + to.x + side_x, kCy + to.y + side_y);

  if (frame.letters > 0) {
    char typed[kWordLetters + 1] = {};
    for (int i = 0; i < frame.letters && i < kWordLetters; ++i) typed[i] = kWord[i];
    u8g2.setFont(u8g2_font_helvR14_tr);
    u8g2.setFontPosCenter();
    u8g2.drawStr(kWordX, kCy, typed);
  }
}

bool boot_animation_step() {
  if (!g_ready) return true;
  static std::uint32_t started_at = 0;
  static bool started = false;
  if (!started) {
    started_at = millis();
    started = true;
  }
  const leeloo::boot_anim::Frame frame = leeloo::boot_anim::frame_at(millis() - started_at);
  {
    U8g2Lock lock;
    u8g2.clearBuffer();
    draw_boot_frame(frame);
  }
  u8g2.sendBuffer();  // the dump itself paces the frames, about 25 ms each
  return frame.done;
}

void power_save() {
  if (!g_ready) return;
  U8g2Lock lock;
  u8g2.setPowerSave(1);  // the SH1106 display-off command: the panel draws almost nothing
}

void set_contrast(std::uint8_t register_value) {
  if (g_ready) u8g2.setContrast(register_value);
}

std::uint32_t piece_count(const char* word, std::size_t length) {
  if (!g_ready) return 1;
  U8g2Lock lock;
  return leeloo::view::piece_count(u8g2, word, length);
}

// Draws one screen into the frame buffer. The caller holds the u8g2 lock.
static void draw_screen(const leeloo::ui::Ui& ui, const Status& status, const leeloo::app::ReadingFrame& frame) {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.setFontPosTop();
  switch (ui.screen()) {
    case Screen::Root: draw_root(ui); break;
    case Screen::Read: draw_read(status, ui); break;
    case Screen::Reading: draw_reading(frame); break;
    case Screen::ReadEnd: draw_read_end(ui); break;
    case Screen::Manage: draw_manage(ui); break;
    case Screen::Upload: draw_upload(status); break;
    case Screen::DeleteList: draw_delete_list(ui); break;
    case Screen::DeleteConfirm: draw_delete_confirm(ui); break;
    case Screen::Settings: draw_settings(ui); break;
    case Screen::Brightness: draw_brightness(ui); break;
    case Screen::Wifi: draw_wifi_setting(ui); break;
    case Screen::SpeedStep: draw_speed_step(ui); break;
    case Screen::About: draw_about(status, ui); break;
    case Screen::Message: draw_message(ui); break;
    case Screen::RecordMenu: draw_record_menu(status, ui); break;
    case Screen::Recording: draw_recording(status, ui); break;
    case Screen::RecordingUpload: draw_upload(status, "ENVIAR AL MÓVIL"); break;
    case Screen::RecordingDeleteConfirm: draw_recording_delete_confirm(ui); break;
    case Screen::PowerConfirm: draw_power_confirm(ui); break;
    case Screen::PoweringOff: draw_powering_off(); break;
  }
}

// Frame `offset` pixels into a slide. Forward: the old screen leaves to the left and
// the new one enters from the right. Backwards it is the mirror image.
static void compose_slide_frame(std::size_t offset, bool backwards) {
  std::uint8_t* out = u8g2.getBufferPtr();
  for (std::size_t page = 0; page < kFramePages; ++page) {
    const std::size_t base = page * kFrameWidth;
    for (std::size_t x = 0; x < kFrameWidth; ++x) {
      if (backwards) {
        out[base + x] = x >= offset ? g_previous_frame[base + x - offset]
                                    : g_next_frame[base + x + kFrameWidth - offset];
      } else {
        const std::size_t source = x + offset;
        out[base + x] = source < kFrameWidth ? g_previous_frame[base + source]
                                             : g_next_frame[base + source - kFrameWidth];
      }
    }
  }
}

void render(const leeloo::ui::Ui& ui, const Status& status, const leeloo::app::ReadingFrame& frame) {
  if (!g_ready) return;
  // The list scroll position belongs to one list; start from the top on a new screen.
  if (g_list_screen != ui.screen()) {
    g_list_start = 0;
    g_list_screen = ui.screen();
  }
  {
    U8g2Lock lock;
    draw_screen(ui, status, frame);
    std::memcpy(g_next_frame, u8g2.getBufferPtr(), kFrameBytes);
  }
  // Reading is never animated, in or out, so the slide cannot delay a word.
  const bool animate = g_have_previous_frame && ui.screen() != g_previous_screen &&
                       ui.screen() != Screen::Reading && g_previous_screen != Screen::Reading;
  if (animate) {
    for (std::size_t step = 1; step <= kSlideSteps; ++step) {
      {
        U8g2Lock lock;
        compose_slide_frame(step * (kFrameWidth / kSlideSteps), ui.back_navigation());
      }
      u8g2.sendBuffer();  // outside the lock: it touches neither the font nor the drawing state
    }
  } else {
    u8g2.sendBuffer();
  }
  std::memcpy(g_previous_frame, g_next_frame, kFrameBytes);
  g_previous_screen = ui.screen();
  g_have_previous_frame = true;
}

}  // namespace leeloo::display
