#include "display_ui.h"

#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>

#include <cstdio>
#include <cstring>

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include "reading_frame.h"
#include "rsvp_view.h"
#include "storage_books.h"
#include "text.h"
#include "upload_server.h"

namespace leeloo::display {
namespace {

using leeloo::settings::BrightnessMode;
using leeloo::settings::WifiMode;
using leeloo::ui::Screen;

constexpr int kSdaPin = 8;
constexpr int kSclPin = 9;
constexpr const char* kFirmwareVersion = "0.1.0-dev";

// Grid from DESIGN.md §3.1: title y0-9, rule y11, four rows at 14/26/38/50.
constexpr int kRowY[4] = {14, 26, 38, 50};
constexpr int kRowWidth = 124;  // x 124-127 is the scroll gutter
constexpr std::size_t kVisibleRows = 4;
constexpr std::size_t kTitleChars = 19;  // 6 px per character, 124 px usable

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

void draw_title(const char* text) {
  u8g2.drawUTF8(0, 0, text);
  u8g2.drawHLine(0, 11, 128);
}

void draw_line(int slot, const char* text) { u8g2.drawUTF8(3, kRowY[slot], text); }

// One menu row: label left, optional value right, inverted when focused.
void draw_row(int slot, const char* label, const char* value, bool focused) {
  const int y = kRowY[slot];
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

// Thin track on the right edge; the thumb shows where the window sits in the list.
void draw_gutter(std::size_t count, std::size_t start) {
  if (count <= kVisibleRows) return;
  constexpr int kTop = 13;
  constexpr int kHeight = 50;
  int thumb = static_cast<int>(kHeight * kVisibleRows / count);
  if (thumb < 6) thumb = 6;
  const int thumb_top = kTop + (kHeight - thumb) * static_cast<int>(start) / static_cast<int>(count - kVisibleRows);
  u8g2.drawFrame(124, kTop, 3, kHeight);
  u8g2.drawBox(124, thumb_top, 3, thumb);
}

// Labels for the list rows that are on screen, scrolled to keep the cursor visible.
template <typename LabelFn>
void draw_list(const char* title, std::size_t count, std::size_t cursor, const char* empty, LabelFn label, const char* value) {
  draw_title(title);
  if (count == 0) {
    draw_line(0, empty);
    return;
  }
  g_list_start = leeloo::text::scroll_start(g_list_start, cursor, count, kVisibleRows);
  for (std::size_t slot = 0; slot < kVisibleRows && g_list_start + slot < count; ++slot) {
    const std::size_t i = g_list_start + slot;
    draw_row(static_cast<int>(slot), label(i), value, i == cursor);
  }
  draw_gutter(count, g_list_start);
}

const char* wifi_label(WifiMode mode) {
  switch (mode) {
    case WifiMode::Off: return "Apagado";
    case WifiMode::Auto: return "Automático";
    case WifiMode::On: return "Encendido";
  }
  return "";
}

const char* message_line1(leeloo::ui::Message m) {
  switch (m) {
    case leeloo::ui::Message::WifiOff: return "Wi-Fi desactivado";
    case leeloo::ui::Message::ApStartFailed: return "No se pudo iniciar";
    case leeloo::ui::Message::UploadSaved: return "Libro guardado.";
    case leeloo::ui::Message::UploadTimedOut: return "Carga cerrada";
    case leeloo::ui::Message::DeleteFailed: return "No se pudo borrar.";
    case leeloo::ui::Message::BookOpenFailed: return "No se pudo abrir";
    case leeloo::ui::Message::None: break;
  }
  return "";
}

const char* message_line2(leeloo::ui::Message m) {
  switch (m) {
    case leeloo::ui::Message::WifiOff: return "Actívalo en Ajustes.";
    case leeloo::ui::Message::ApStartFailed: return "el Wi-Fi.";
    case leeloo::ui::Message::UploadSaved: return "";
    case leeloo::ui::Message::UploadTimedOut: return "por inactividad.";
    case leeloo::ui::Message::DeleteFailed: return "El libro se conserva.";
    case leeloo::ui::Message::BookOpenFailed: return "el libro.";
    case leeloo::ui::Message::None: break;
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
    std::snprintf(label, sizeof label, "%lu WPM", static_cast<unsigned long>(frame.wpm));
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
      "BORRAR", ui.book_count(), ui.cursor(), "Sin libros",
      [&label](std::size_t i) {
        book_label(i, label, sizeof label);
        return static_cast<const char*>(label);
      },
      nullptr);
}

void draw_starting() { draw_line(0, "Iniciando..."); }

void draw_upload(const Status& status) {
  draw_title("SUBIR");
  if (!status.ap_running) {
    draw_starting();
    return;
  }
  char pin_line[24];
  std::snprintf(pin_line, sizeof pin_line, "PIN: %s", status.pin);
  char address_line[32];
  std::snprintf(address_line, sizeof address_line, "Dir: %s", leeloo::upload_server::kAddress);
  draw_line(0, leeloo::upload_server::kSsid);
  draw_line(1, pin_line);
  draw_line(2, address_line);
  draw_line(3, "SELECT largo: salir");
}

void draw_message(const leeloo::ui::Ui& ui) {
  draw_line(0, message_line1(ui.message()));
  draw_line(1, message_line2(ui.message()));
  draw_line(3, "Pulsa para volver");
}

void draw_about(const Status& status, const leeloo::ui::Ui& ui) {
  draw_title("INFO");
  draw_row(0, "Firmware", kFirmwareVersion, false);
  char free_kb[16];
  std::snprintf(free_kb, sizeof free_kb, "%u KB", static_cast<unsigned>(status.free_bytes / 1024));
  draw_row(1, "Libre", free_kb, false);
  draw_row(2, "Wi-Fi", wifi_label(ui.settings().wifi), false);
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
  const bool manual = ui.settings().brightness_mode == BrightnessMode::Manual;
  char mode[20];
  format_value(mode, sizeof mode, manual ? "Manual" : "Auto", ui.editing() && ui.cursor() == 0);
  draw_row(0, "Modo", mode, ui.cursor() == 0);
  if (manual) {
    draw_percent_row(1, "Nivel", ui.settings().brightness_percent, ui.cursor() == 1,
                     ui.editing() && ui.cursor() == 1);
  } else if (ui.sensor_unavailable()) {
    draw_line(2, "Sensor no disponible");
  }
}

void draw_contrast(const leeloo::ui::Ui& ui) {
  draw_title("CONTRASTE");
  draw_percent_row(0, "Nivel", ui.settings().contrast_percent, true, ui.editing());
}

void draw_wifi_setting(const leeloo::ui::Ui& ui) {
  draw_title("WI-FI");
  char value[32];
  format_value(value, sizeof value, wifi_label(ui.settings().wifi), ui.editing());
  draw_row(0, "Modo", value, true);
}

void draw_root(const leeloo::ui::Ui& ui) {
  static const char* const kItems[] = {"Leer", "Biblioteca", "Ajustes", "Info"};
  draw_title("LEELOO");
  for (std::size_t i = 0; i < 4; ++i) {
    draw_row(static_cast<int>(i), kItems[i], ">", i == ui.cursor());
  }
}

void draw_manage(const leeloo::ui::Ui& ui) {
  draw_title("BIBLIOTECA");
  draw_row(0, "Subir", ">", ui.cursor() == 0);
  draw_row(1, "Borrar", ">", ui.cursor() == 1);
}

void draw_settings(const leeloo::ui::Ui& ui) {
  draw_title("AJUSTES");
  draw_row(0, "Brillo", ">", ui.cursor() == 0);
  draw_row(1, "Contraste", ">", ui.cursor() == 1);
  draw_row(2, "Wi-Fi", ">", ui.cursor() == 2);
}

void draw_delete_confirm(const leeloo::ui::Ui& ui) {
  draw_title("BORRAR");
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
    case Screen::Contrast: draw_contrast(ui); break;
    case Screen::Wifi: draw_wifi_setting(ui); break;
    case Screen::About: draw_about(status, ui); break;
    case Screen::Message: draw_message(ui); break;
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
  }
  u8g2.sendBuffer();  // outside the lock: it touches neither the font nor the drawing state
}

}  // namespace leeloo::display
