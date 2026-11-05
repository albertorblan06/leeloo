#include "rsvp_view.h"

#include <cstring>

#include "rsvp.h"
#include "text.h"

namespace leeloo::view {
namespace {

constexpr int kWidth = 128;
constexpr int kCenterX = kWidth / 2;
constexpr int kCenterY = 32;
constexpr std::size_t kMaxWordChars = 18;  // wider words are cut with "..."
constexpr std::size_t kClipCap = 80;       // 18 code points of up to 4 bytes, plus NUL
constexpr std::size_t kPieceCap = 80;      // a piece is a slice of the clipped word, plus "-"
// A clipped word has at most 18 code points, so it splits into at most 18 pieces.
constexpr std::size_t kMaxPieces = 24;

// One Latin-1 font at 18 pt. Words that do not fit are split at a syllable
// boundary instead of shrinking the font.
const uint8_t* const kFont = u8g2_font_helvB18_tf;

// The focal letter is drawn hollow: a white contour around the glyph with a
// black inside. kOutlineGap keeps the contour from touching the neighbors.
constexpr int kOutlineWidth = 1;
constexpr int kOutlineGap = 1;
constexpr int kOutlineMargin = kOutlineWidth + kOutlineGap;

// Word font and transparent mode for the duration of one word; the menu font is
// restored afterwards, since the menus draw without setting it themselves.
class WordFont {
 public:
  explicit WordFont(U8G2& u8g2) : u8g2_(u8g2) {
    u8g2_.setFont(kFont);
    u8g2_.setFontMode(1);  // transparent: the shifted contour copies must not erase neighbors
    u8g2_.setFontPosCenter();
  }
  ~WordFont() {
    u8g2_.setFontMode(0);
    u8g2_.setFont(u8g2_font_6x10_tf);
    u8g2_.setFontPosTop();
  }
  WordFont(const WordFont&) = delete;
  WordFont& operator=(const WordFont&) = delete;

 private:
  U8G2& u8g2_;
};

// A slice of the clipped word. `hyphen` marks a first half: a "-" follows it.
struct Piece {
  const char* text;
  std::size_t length;
  bool hyphen;
};

// Copies `count` bytes into `dst` and NUL-terminates it. U8g2 only measures and
// draws NUL-terminated strings, so every part goes through here.
void copy_slice(char* dst, const char* src, std::size_t count) {
  std::memcpy(dst, src, count);
  dst[count] = '\0';
}

// Where each part of a piece goes. The hollow focal letter always sits on the
// center column (x = 64), so there is no fallback that re-centers the piece.
struct Layout {
  char prefix[kPieceCap];
  char focal[kPieceCap];
  char suffix[kPieceCap];
  int prefix_x;
  int focal_x;
  int suffix_x;
  bool fits;  // whole piece is visible with the focal letter on the center column
};

void layout_piece(U8G2& u8g2, const Piece& piece, Layout* out) {
  char text[kPieceCap];
  copy_slice(text, piece.text, piece.length);
  std::size_t text_len = piece.length;
  if (piece.hyphen) {
    text[text_len++] = '-';
    text[text_len] = '\0';
  }
  const auto split = rsvp::focal_split(text, piece.length);  // the hyphen is never the focal letter
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

bool fits(U8G2& u8g2, const Piece& piece) {
  Layout layout;
  layout_piece(u8g2, piece, &layout);
  return layout.fits;
}

// U8g2 coordinates are unsigned, so a negative x would wrap around; parts that
// start off screen are skipped instead.
// shortcut: a piece that cannot be split and does not fit loses its off-screen
// parts, upgrade if book text can contain such words.
void draw_at(U8G2& u8g2, int x, int y, const char* text) {
  if (x < 0 || x >= kWidth) return;
  u8g2.drawUTF8(x, y, text);
}

void draw_layout(U8G2& u8g2, const Layout& layout) {
  u8g2.setDrawColor(1);
  u8g2.drawVLine(kCenterX, 6, 6);  // focal marks above and below the word
  u8g2.drawVLine(kCenterX, 52, 6);
  draw_at(u8g2, layout.prefix_x, kCenterY, layout.prefix);
  draw_at(u8g2, layout.suffix_x, kCenterY, layout.suffix);

  // Hollow focal letter: white copies shifted around the glyph form its contour,
  // then the glyph in black clears the inside.
  for (int dy = -kOutlineWidth; dy <= kOutlineWidth; ++dy) {
    for (int dx = -kOutlineWidth; dx <= kOutlineWidth; ++dx) {
      if (dx != 0 || dy != 0) draw_at(u8g2, layout.focal_x + dx, kCenterY + dy, layout.focal);
    }
  }
  u8g2.setDrawColor(0);
  draw_at(u8g2, layout.focal_x, kCenterY, layout.focal);
  u8g2.setDrawColor(1);
}

// Splits the span like the RSVP view always has: a span that fits stays whole,
// otherwise it is cut at a syllable boundary and both halves are checked again.
// A span with no boundary is kept whole and clipped. Returns the piece count.
std::size_t collect(U8G2& u8g2, const char* word, std::size_t length, bool hyphen, Piece* out, std::size_t count) {
  if (length == 0 || count >= kMaxPieces) return count;
  const Piece whole{word, length, hyphen};
  if (fits(u8g2, whole)) {
    out[count++] = whole;
    return count;
  }
  const std::size_t cut = rsvp::split_point(word, length);
  if (cut == 0) {
    out[count++] = whole;
    return count;
  }
  count = collect(u8g2, word, cut, true, out, count);
  return collect(u8g2, word + cut, length - cut, hyphen, out, count);
}

// Clips the word to the displayed width. Returns the bytes written, 0 when the
// word cannot be shown (invalid UTF-8).
std::size_t clip(const char* word, std::size_t length, char* out) {
  return text::truncate_chars(word, length, kMaxWordChars, "...", out, kClipCap);
}

}  // namespace

std::uint32_t piece_count(U8G2& u8g2, const char* word, std::size_t length) {
  WordFont font(u8g2);
  char clipped[kClipCap];
  const std::size_t n = clip(word, length, clipped);
  Piece pieces[kMaxPieces];
  return static_cast<std::uint32_t>(collect(u8g2, clipped, n, false, pieces, 0));
}

void draw_piece(U8G2& u8g2, const char* word, std::size_t length, std::uint32_t piece) {
  WordFont font(u8g2);
  char clipped[kClipCap];
  const std::size_t n = clip(word, length, clipped);
  if (n == 0) return;
  Piece pieces[kMaxPieces];
  const std::size_t count = collect(u8g2, clipped, n, false, pieces, 0);
  if (count == 0) return;
  const std::size_t index = piece < count ? piece : count - 1;
  Layout layout;
  layout_piece(u8g2, pieces[index], &layout);
  draw_layout(u8g2, layout);
}

}  // namespace leeloo::view
