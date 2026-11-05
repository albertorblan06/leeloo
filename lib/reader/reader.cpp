#include "reader.h"

#include <algorithm>
#include <string_view>

#include "upload.h"

namespace leeloo::reader {

namespace {

// Bounds the sentence scans, so a book with no sentence marks cannot stall the
// loop. shortcut: a sentence longer than 200 words stops early; upgrade to a
// streaming scan if such passages turn up in real books.
constexpr std::size_t kMaxSentenceWords = 200;

// Same word separators as rsvp::next_word, so forward and backward steps agree.
// UTF-8 continuation and lead bytes are never ASCII, so a backwards byte scan
// cannot stop inside a code point.
bool is_space(char byte) {
  return byte == ' ' || byte == '\t' || byte == '\n' || byte == '\r';
}

bool is_continuation(char byte) {
  return (static_cast<unsigned char>(byte) & 0xC0) == 0x80;
}

// Start of the word before `before`, or nullopt when there is none.
std::optional<std::size_t> previous_word_start(const char* text, std::size_t before) {
  std::size_t pos = before;
  while (pos > 0 && is_space(text[pos - 1])) --pos;
  if (pos == 0) return std::nullopt;
  while (pos > 0 && !is_space(text[pos - 1])) --pos;
  return pos;
}

}  // namespace

std::optional<std::size_t> body_offset(const char* text, std::size_t size) {
  const std::string_view marker = upload::kStartMarker;
  const std::string_view all(text, size);
  std::size_t from = 0;
  while (from < size) {
    const std::size_t at = all.find(marker, from);
    if (at == std::string_view::npos) return std::nullopt;
    const std::size_t end = at + marker.size();
    const bool own_line = (at == 0 || text[at - 1] == '\n') && end < size && text[end] == '\n';
    if (own_line) return end + 1;
    from = at + 1;
  }
  return std::nullopt;
}

Reader::Reader(const char* body, std::size_t size, std::size_t resume_at, std::uint32_t now_ms,
               PieceCounter pieces)
    : body_(body), size_(size), pieces_fn_(pieces) {
  const bool usable = resume_at < size && !is_continuation(body[resume_at]);
  const std::optional<rsvp::WordSpan> first = rsvp::next_word(body_, size_, usable ? resume_at : 0);
  if (!first) {
    state_ = State::Finished;
    return;
  }
  show(*first, now_ms, now_ms);
}

bool Reader::tick(std::uint32_t now_ms) {
  if (state_ != State::Reading) return false;
  // Signed differences, so the comparisons survive a 32-bit millis() wraparound.
  if (static_cast<std::int32_t>(now_ms - due_ms_) < 0) {
    const auto elapsed = static_cast<std::int32_t>(now_ms - start_ms_);
    if (elapsed < 0 || pieces_ <= 1) return false;
    const auto index = std::min(static_cast<std::uint32_t>(elapsed) / beat_ms_, pieces_ - 1);
    if (index == piece_) return false;
    piece_ = index;
    return true;
  }
  const std::size_t after = current_->start + current_->length;
  const std::optional<rsvp::WordSpan> next = rsvp::next_word(body_, size_, after);
  if (!next) {
    state_ = State::Finished;
    return true;
  }
  show(*next, due_ms_, now_ms);
  return true;
}

std::uint32_t Reader::wake_ms() const {
  if (state_ == State::Reading && piece_ + 1 < pieces_) return start_ms_ + beat_ms_ * (piece_ + 1);
  return due_ms_;
}

void Reader::pause() {
  if (state_ != State::Reading) return;
  state_ = State::Paused;
  piece_ = 0;
}

void Reader::resume(std::uint32_t now_ms) {
  if (state_ != State::Paused) return;
  state_ = State::Reading;
  show(*current_, now_ms, now_ms);
}

void Reader::faster() {
  if (wpm_ < rsvp::kMaxWpm) wpm_ += rsvp::kWpmStep;
}

void Reader::slower() {
  if (wpm_ > rsvp::kMinWpm) wpm_ -= rsvp::kWpmStep;
}

void Reader::step_sentence(int direction) {
  if (state_ != State::Paused || !current_) return;
  if (direction < 0) {
    const std::size_t here = current_->start;
    const std::size_t start = sentence_start_at(here);
    if (start != here) {
      if (const auto word = rsvp::next_word(body_, size_, start)) current_ = word;
      return;
    }
    // Already at a sentence start: go to the start of the previous sentence.
    const auto prev = word_before(here);
    if (!prev) return;
    if (const auto word = rsvp::next_word(body_, size_, sentence_start_at(prev->start))) current_ = word;
    return;
  }
  // Forward: find this sentence's end, then step onto the word after it.
  std::optional<rsvp::WordSpan> scan = current_;
  for (std::size_t n = 0; n < kMaxSentenceWords && scan; ++n) {
    if (rsvp::ends_sentence(body_, size_, *scan)) {
      if (const auto after = rsvp::next_word(body_, size_, scan->start + scan->length)) current_ = after;
      return;
    }
    scan = rsvp::next_word(body_, size_, scan->start + scan->length);
  }
  // No sentence end within the limit: move to where the scan stopped.
  if (scan) current_ = scan;
}

std::size_t Reader::offset() const {
  if (state_ == State::Finished || !current_) return 0;
  return current_->start;
}

std::uint32_t Reader::progress_percent() const {
  if (size_ == 0) return 0;
  return static_cast<std::uint32_t>(offset() * 100 / size_);
}

std::uint32_t Reader::hold_ms(const rsvp::WordSpan& span) const {
  const std::uint32_t punctuation = rsvp::hold_percent(body_, size_, span);
  return beat_ms_ * (pieces_ - 1) + beat_ms_ * punctuation / 100;
}

// Keeps the planned pace while the loop is less than one hold late, so redraw
// time does not slow the WPM down. A longer stall restarts the pace from now.
void Reader::show(const rsvp::WordSpan& span, std::uint32_t planned_ms, std::uint32_t now_ms) {
  current_ = span;
  beat_ms_ = rsvp::word_delay_ms(wpm_);
  pieces_ = pieces_fn_ ? std::max<std::uint32_t>(1, pieces_fn_(body_ + span.start, span.length)) : 1;
  piece_ = 0;
  const std::uint32_t hold = hold_ms(span);
  const bool on_pace = static_cast<std::int32_t>(now_ms - planned_ms) < static_cast<std::int32_t>(hold);
  start_ms_ = on_pace ? planned_ms : now_ms;
  due_ms_ = start_ms_ + hold;
}

std::optional<rsvp::WordSpan> Reader::word_before(std::size_t pos) const {
  const auto start = previous_word_start(body_, pos);
  if (!start) return std::nullopt;
  return rsvp::next_word(body_, size_, *start);
}

// Start of the sentence that contains the word beginning at `pos`: the word right
// after the nearest sentence end before it, or the first word of the book.
std::size_t Reader::sentence_start_at(std::size_t pos) const {
  std::size_t start = pos;
  for (std::size_t n = 0; n < kMaxSentenceWords; ++n) {
    const auto prev = word_before(start);
    if (!prev || rsvp::ends_sentence(body_, size_, *prev)) break;
    start = prev->start;
  }
  return start;
}

}  // namespace leeloo::reader
