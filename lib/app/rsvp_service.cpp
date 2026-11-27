#include "rsvp_service.h"

#include <algorithm>

#include "text.h"

namespace leeloo::app {

namespace {

// Milliseconds from `now` to `at`, 0 when already past. Correct across a 32-bit wrap.
std::uint32_t until(std::uint32_t at, std::uint32_t now) {
  const auto delta = static_cast<std::int32_t>(at - now);
  return delta <= 0 ? 0 : static_cast<std::uint32_t>(delta);
}

}  // namespace

RsvpService::RsvpService(ILibrary* library, rtos::Queue<ui::Event>* ui_out,
                         reader::PieceCounter pieces, ISpeedStore* speed, ILog* log)
    : library_(library),
      ui_out_(ui_out),
      pieces_(pieces),
      speed_(speed),
      log_(log),
      last_wpm_(speed != nullptr ? speed->load() : rsvp::kDefaultWpm) {}

void RsvpService::close_book() {
  reader_.reset();
  label_until_ms_ = 0;
  library_->close();
}

// Stores the word the reader stopped on. A finished reader offers offset 0, so
// the next open starts at the beginning.
void RsvpService::save_progress() {
  if (!reader_) return;
  if (!library_->save_position(title_, reader_->offset()) && log_ != nullptr) {
    log_->log("Lectura: no se pudo guardar la posición");
  }
}

// Saves the reader's speed when it differs from the stored one. Not called on pause:
// the speed is a per-exit preference, not per-pause, and NVS writes are limited.
void RsvpService::save_speed() {
  // Only a user change is saved: opening re-aligns the speed to the step, which must not overwrite the stored value.
  if (!reader_ || speed_ == nullptr || !speed_dirty_ || reader_->wpm() == last_wpm_) return;
  const auto wpm = static_cast<std::uint16_t>(reader_->wpm());
  if (speed_->save(wpm)) {
    last_wpm_ = wpm;
    speed_dirty_ = false;
  } else if (log_ != nullptr) {
    log_->log("Lectura: no se pudo guardar la velocidad");
  }
}

// The last word has been held: the position resets to 0, then the book closes.
void RsvpService::finish_book() {
  save_progress();
  save_speed();
  close_book();
  post(ui_out_, ui::Event::ReaderFinished, log_, "Cola de UI llena: se pierde ReaderFinished");
}

// Loads book `index` at its saved word. False when the text cannot be used.
bool RsvpService::open_book(std::size_t index, std::uint32_t now_ms, std::uint32_t wpm_step) {
  close_book();
  speed_dirty_ = false;
  BookBuffer buffer;
  if (!library_->open(index, title_, sizeof title_, &buffer)) return false;
  const auto body = reader::body_offset(buffer.data, buffer.size);
  if (!body) {
    close_book();
    return false;
  }
  std::size_t saved = 0;
  if (!library_->load_position(title_, &saved)) saved = 0;
  reader_.emplace(buffer.data + *body, buffer.size - *body, saved, now_ms, pieces_, last_wpm_,
                  wpm_step);
  return true;
}

bool RsvpService::handle(const RsvpMsg& msg, std::uint32_t now_ms) {
  switch (msg.cmd) {
    case RsvpCmd::Open:
      if (!open_book(msg.index, now_ms, msg.wpm_step)) {
        post(ui_out_, ui::Event::BookOpenFailed, log_, "Cola de UI llena: se pierde BookOpenFailed");
        return true;
      }
      // An empty book has nothing to hold, so it ends at once.
      if (reader_->state() == reader::State::Finished) finish_book();
      return true;
    case RsvpCmd::Close:
      save_progress();
      save_speed();
      close_book();
      return true;
    case RsvpCmd::Select:
      if (!reader_) return false;
      if (reader_->state() == reader::State::Reading) {
        reader_->pause();
        save_progress();
      } else {
        reader_->resume(now_ms);
      }
      return true;
    case RsvpCmd::Up:
    case RsvpCmd::Down: {
      if (!reader_) return false;
      const bool up = msg.cmd == RsvpCmd::Up;
      if (reader_->state() == reader::State::Paused) {
        reader_->step_sentence(up ? -1 : +1);
        save_progress();  // a seek is kept even if power is lost while paused
        return true;
      }
      change_speed(up, now_ms);
      return true;
    }
    case RsvpCmd::UpRepeat:
    case RsvpCmd::DownRepeat:
      if (!reader_ || reader_->state() != reader::State::Reading) return false;
      change_speed(msg.cmd == RsvpCmd::UpRepeat, now_ms);
      return true;
  }
  return false;
}

void RsvpService::change_speed(bool faster, std::uint32_t now_ms) {
  if (faster) {
    reader_->faster();
  } else {
    reader_->slower();
  }
  speed_dirty_ = true;
  label_until_ms_ = now_ms + kWpmLabelMs;
}

bool RsvpService::on_timeout(std::uint32_t now_ms) {
  bool changed = false;
  if (reader_ && reader_->tick(now_ms)) {
    if (reader_->state() == reader::State::Finished) finish_book();
    changed = true;
  }
  // Hides the WPM label once its time is up.
  if (label_until_ms_ != 0 && static_cast<std::int32_t>(now_ms - label_until_ms_) >= 0) {
    label_until_ms_ = 0;
    changed = true;
  }
  return changed;
}

std::uint32_t RsvpService::wait_ms(std::uint32_t now_ms) const {
  std::uint32_t wait = rtos::kWaitForever;
  if (reader_ && reader_->state() == reader::State::Reading) {
    wait = until(reader_->wake_ms(), now_ms);
  }
  if (label_until_ms_ != 0) wait = std::min(wait, until(label_until_ms_, now_ms));
  return wait;
}

ReadingFrame RsvpService::frame() const {
  ReadingFrame out;
  if (!reader_) return out;
  out.active = true;
  out.state = reader_->state();
  out.wpm = reader_->wpm();
  out.piece = reader_->piece();
  out.show_wpm = label_until_ms_ != 0;
  if (const auto span = reader_->word()) {
    // An invalid word leaves the buffer empty; the view draws nothing for it.
    text::truncate_chars(reader_->body() + span->start, span->length, kFrameWordChars, "...",
                         out.word, sizeof out.word);
  }
  return out;
}

}  // namespace leeloo::app
