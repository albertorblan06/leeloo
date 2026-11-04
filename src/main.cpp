#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>

#include "rsvp.h"

// Smoke test: shows a hardcoded phrase word by word (RSVP) on the OLED.
// Hardware-bound, so it is covered by docs/hardware-checklists/oled-rsvp-smoke.md.

namespace {

constexpr int kSdaPin = 8;
constexpr int kSclPin = 9;
constexpr std::uint32_t kWpm = 200;
constexpr char kPhrase[] =
    "Empezamos por la alimentación general. Llevamos cinco voltios y tierra desde el Arduino "
    "directamente a los raíles laterales de la placa. ¿Por qué? Para centralizar la distribución "
    "de energía. Evitamos tirar cables individuales desde el microcontrolador a cada componente "
    "porque eso genera ruido eléctrico y puntos de fallo.\n\n"
    "Los raíles actúan como un bus de "
    "alimentación limpio, estableciendo un voltaje de referencia estable para todo el sistema físico.";
constexpr std::uint32_t kLoopPauseMs = 1000;

constexpr int kWidth = 128;
constexpr int kCenterX = kWidth / 2;
constexpr int kCenterY = 32;
constexpr std::size_t kMaxWordBytes = 48;

// One Latin-1 (_tf) font at 18 pt. Words that do not fit are split at a
// syllable boundary instead of shrinking the font.
const uint8_t* const kFont = u8g2_font_helvB18_tf;

// The focal letter is drawn hollow: a white contour around the glyph with a
// black inside. kOutlineGap keeps the contour from touching the neighbors.
constexpr int kOutlineWidth = 1;
constexpr int kOutlineGap = 1;
constexpr int kOutlineMargin = kOutlineWidth + kOutlineGap;

// SH1106 is only the likely controller (see docs/ARCHITECTURE.md). If the
// picture is shifted 2 px or has garbage at the edges, switch to
// U8G2_SSD1306_128X64_NONAME_F_HW_I2C.
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
bool g_display_ready = false;

bool i2c_device_present(std::uint8_t address) {
  Wire.beginTransmission(address);
  return Wire.endTransmission() == 0;
}

// Prints every responding address and returns the OLED's (0x3C or 0x3D), or 0.
std::uint8_t scan_i2c_for_oled() {
  std::uint8_t oled = 0;
  for (std::uint8_t address = 1; address < 127; ++address) {
    if (!i2c_device_present(address)) continue;
    Serial.printf("I2C device at 0x%02X\n", address);
    if (oled == 0 && (address == 0x3C || address == 0x3D)) oled = address;
  }
  return oled;
}

// Copies `count` bytes into `dst` and NUL-terminates it. U8g2 only measures
// and draws NUL-terminated strings, so every piece goes through here.
void copy_slice(char* dst, const char* src, std::size_t count) {
  memcpy(dst, src, count);
  dst[count] = '\0';
}

// Builds the text to show for a piece: the piece itself, plus "-" when it is
// the first half of a split word. Returns the text length.
std::size_t build_text(const char* word, std::size_t length, bool hyphen, char* text) {
  copy_slice(text, word, length);
  if (!hyphen) return length;
  text[length] = '-';
  text[length + 1] = '\0';
  return length + 1;
}

// Where each part of a piece goes. The hollow focal letter is always the one
// on the center column (x = 64), as RSVP requires, so there is no fallback
// that re-centers the whole piece.
struct Layout {
  char prefix[kMaxWordBytes];
  char focal[kMaxWordBytes];
  char suffix[kMaxWordBytes];
  int prefix_x;
  int focal_x;
  int suffix_x;
  bool fits;  // whole piece is visible with the focal letter on the center column
};

void layout_piece(const char* word, std::size_t length, bool hyphen, Layout* out) {
  char text[kMaxWordBytes];
  const std::size_t text_len = build_text(word, length, hyphen, text);
  const auto split = leeloo::rsvp::focal_split(text, length);  // hyphen is not a focal candidate
  copy_slice(out->prefix, text, split.prefix_bytes);
  copy_slice(out->focal, text + split.prefix_bytes, split.focal_bytes);
  const std::size_t suffix_start = split.prefix_bytes + split.focal_bytes;
  copy_slice(out->suffix, text + suffix_start, text_len - suffix_start);

  const int prefix_w = u8g2.getUTF8Width(out->prefix);
  const int focal_w = u8g2.getUTF8Width(out->focal);
  const int total_w = u8g2.getUTF8Width(text) + 2 * kOutlineMargin;
  out->prefix_x = kCenterX - prefix_w - kOutlineMargin - focal_w / 2;
  out->focal_x = out->prefix_x + prefix_w + kOutlineMargin;
  out->suffix_x = out->focal_x + focal_w + kOutlineMargin;
  out->fits = out->prefix_x >= 0 && out->prefix_x + total_w <= kWidth;
}

bool fits(const char* word, std::size_t length, bool hyphen) {
  Layout layout;
  layout_piece(word, length, hyphen, &layout);
  return layout.fits;
}

// U8g2 coordinates are unsigned, so a negative x would wrap around; parts
// that start off screen are skipped instead.
// shortcut: a piece that cannot be split and does not fit loses its off-screen
// parts, upgrade when the book text can contain such words.
void draw_at(int x, int y, const char* text) {
  if (x < 0 || x >= kWidth) return;
  u8g2.drawUTF8(x, y, text);
}

// Draws one piece with its hollow focal letter on the center column.
void draw_piece(const char* word, std::size_t length, bool hyphen) {
  Layout layout;
  layout_piece(word, length, hyphen, &layout);

  u8g2.clearBuffer();
  u8g2.setFontPosCenter();
  u8g2.setDrawColor(1);
  u8g2.drawVLine(kCenterX, 6, 6);   // focal marks above and below the word
  u8g2.drawVLine(kCenterX, 52, 6);
  draw_at(layout.prefix_x, kCenterY, layout.prefix);
  draw_at(layout.suffix_x, kCenterY, layout.suffix);

  // Hollow focal letter: white copies shifted around the glyph form its
  // contour, then the glyph in black clears the inside.
  for (int dy = -kOutlineWidth; dy <= kOutlineWidth; ++dy) {
    for (int dx = -kOutlineWidth; dx <= kOutlineWidth; ++dx) {
      if (dx != 0 || dy != 0) draw_at(layout.focal_x + dx, kCenterY + dy, layout.focal);
    }
  }
  u8g2.setDrawColor(0);
  draw_at(layout.focal_x, kCenterY, layout.focal);
  u8g2.setDrawColor(1);
  u8g2.sendBuffer();
}

// Shows one piece for `hold` percent of a word period at kWpm, minus the time
// the redraw took (I2C redraw is ~25 ms). 100 = one normal beat.
void present(const char* word, std::size_t length, bool hyphen, std::uint32_t hold) {
  const std::uint32_t period_ms = leeloo::rsvp::word_delay_ms(kWpm) * hold / 100;
  const std::uint32_t started = millis();
  draw_piece(word, length, hyphen);
  const std::uint32_t elapsed = millis() - started;
  if (elapsed < period_ms) delay(period_ms - elapsed);
}

// Shows a word, split at a syllable boundary until every piece fits with its
// focal letter on the center column. Only the last piece keeps the word's
// `hold` (that is where its punctuation lives); earlier pieces get a normal beat.
// `hyphen` marks a piece followed by the rest of its word.
void show_word(const char* word, std::size_t length, bool hyphen, std::uint32_t hold) {
  if (length == 0 || length + 1 >= kMaxWordBytes) return;  // phrase words are short; never cut UTF-8

  if (fits(word, length, hyphen)) {
    present(word, length, hyphen, hold);
    return;
  }
  const std::size_t cut = leeloo::rsvp::split_point(word, length);
  if (cut == 0) {  // one syllable and too wide: show it clipped rather than break it
    present(word, length, hyphen, hold);
    return;
  }
  show_word(word, cut, true, 100);
  show_word(word + cut, length - cut, hyphen, hold);
}

}  // namespace

void setup() {
  Serial.begin(115200);
  Wire.begin(kSdaPin, kSclPin);

  const std::uint8_t address = scan_i2c_for_oled();
  if (address == 0) {
    Serial.println("No OLED found at 0x3C/0x3D. Check wiring: SDA=GPIO8, SCL=GPIO9.");
    return;
  }
  u8g2.setI2CAddress(address << 1);  // U8g2 wants the 8-bit form
  u8g2.begin();
  u8g2.setBusClock(400000);
  u8g2.enableUTF8Print();
  u8g2.setFont(kFont);
  u8g2.setFontMode(1);  // transparent: shifted copies must not erase their neighbors
  g_display_ready = true;
}

void loop() {
  if (!g_display_ready) {
    delay(1000);
    return;
  }

  const std::size_t size = sizeof(kPhrase) - 1;
  std::size_t offset = 0;
  while (const auto span = leeloo::rsvp::next_word(kPhrase, size, offset)) {
    const std::uint32_t hold = leeloo::rsvp::hold_percent(kPhrase, size, *span);
    show_word(kPhrase + span->start, span->length, false, hold);
    offset = span->start + span->length;
  }

  u8g2.clearBuffer();
  u8g2.sendBuffer();
  delay(kLoopPauseMs);
}
