#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>

#include "rsvp.h"

// Pure RSVP playback for one book (docs/DEVICE-MENU.md "Leer", ARCHITECTURE.md
// §RSVP). No Arduino, no I/O: the adapter in src/ owns the text buffer and the
// clock, and passes the current time in.
namespace leeloo::reader {

// Used when a book has no saved speed; the settings menu does not store WPM yet.
constexpr std::uint32_t kDefaultWpm = 300;

// Byte offset of the book text: the first byte after the "###START###" line.
// nullopt when the header has no such line, e.g. a damaged file.
std::optional<std::size_t> body_offset(const char* text, std::size_t size);

// How many pieces a word is shown in: 1 when it fits, more when it is split at
// syllable boundaries. Supplied by the adapter, which knows the display width.
using PieceCounter = std::uint32_t (*)(const char* word, std::size_t length);

enum class State : std::uint8_t {
  Reading,
  Paused,
  Finished,  // the last word has been held; the offset resets to 0
};

// `body` must outlive the reader. Offsets are relative to `body`.
class Reader {
 public:
  // `resume_at` is a saved word offset. An offset that is out of range or inside
  // a UTF-8 sequence starts the book from its first word instead. `pieces`
  // may be null, which shows every word whole.
  Reader(const char* body, std::size_t size, std::size_t resume_at, std::uint32_t now_ms,
         PieceCounter pieces = nullptr);

  // Advances past the current word once its hold has elapsed, and moves to the
  // next piece of a split word when its beat has passed. Returns true when the
  // shown word, piece or state changed, so the caller knows to redraw.
  bool tick(std::uint32_t now_ms);

  // Absolute time of the next change: the next piece boundary, or the end of the
  // hold, whichever is first. Only meaningful while Reading.
  std::uint32_t wake_ms() const;

  void pause();
  void resume(std::uint32_t now_ms);  // restarts the word from its first piece; no effect once finished

  // Speed changes apply from the next word on. Clamped to the RSVP WPM range.
  void faster();
  void slower();

  // Paused only: moves to the first word of the next (+1) or previous (-1)
  // sentence. Stops at the book's ends.
  void step_sentence(int direction);

  State state() const { return state_; }
  std::uint32_t wpm() const { return wpm_; }
  const char* body() const { return body_; }
  std::size_t size() const { return size_; }
  std::optional<rsvp::WordSpan> word() const { return current_; }

  // Piece being shown: always 0 while paused, since a paused reader shows the
  // first piece of the word it stopped on.
  std::uint32_t piece() const { return piece_; }

  // Byte offset of the shown word, to be saved as the resume point. Zero once finished.
  std::size_t offset() const;

  // How far into the book the shown word lies, 0-100.
  std::uint32_t progress_percent() const;

 private:
  std::uint32_t hold_ms(const rsvp::WordSpan& span) const;
  void show(const rsvp::WordSpan& span, std::uint32_t planned_ms, std::uint32_t now_ms);
  std::optional<rsvp::WordSpan> word_before(std::size_t pos) const;
  std::size_t sentence_start_at(std::size_t pos) const;

  const char* body_;
  std::size_t size_;
  PieceCounter pieces_fn_;
  std::optional<rsvp::WordSpan> current_;
  std::uint32_t wpm_ = kDefaultWpm;
  std::uint32_t due_ms_ = 0;
  std::uint32_t start_ms_ = 0;   // when the shown word's first beat began (on pace)
  std::uint32_t beat_ms_ = 0;    // one beat of the shown word, fixed when it is shown
  std::uint32_t pieces_ = 1;     // pieces of the shown word
  std::uint32_t piece_ = 0;      // piece on screen
  State state_ = State::Reading;
};

}  // namespace leeloo::reader
