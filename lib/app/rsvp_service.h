#pragma once

#include <cstdint>
#include <optional>

#include "messages.h"
#include "ports.h"
#include "reader.h"
#include "reading_frame.h"
#include "rtos.h"
#include "ui.h"

namespace leeloo::app {

// Owns the open book and its Reader. UiService sends commands; this service holds
// each word for its time and tells the UI when the book opens badly or ends.
// The RSVP task blocks for wait_ms() on its command queue, calls handle() for each
// command and on_timeout() when the wait runs out.
class RsvpService {
 public:
  // How long the WPM label stays on screen after a speed change.
  static constexpr std::uint32_t kWpmLabelMs = 2000;

  RsvpService(ILibrary* library, rtos::Queue<ui::Event>* ui_out, reader::PieceCounter pieces,
              ILog* log = nullptr);

  // Both return true when the frame changed and the display needs a new one.
  bool handle(const RsvpMsg& msg, std::uint32_t now_ms);
  bool on_timeout(std::uint32_t now_ms);

  // kWaitForever when no word is due and no label is up; otherwise the time to the next change.
  std::uint32_t wait_ms(std::uint32_t now_ms) const;

  // Copy of what the screen shows now. `active` is false when no book is open.
  ReadingFrame frame() const;

 private:
  bool open_book(std::size_t index, std::uint32_t now_ms);
  void close_book();
  void save_progress();
  void finish_book();

  ILibrary* library_;
  rtos::Queue<ui::Event>* ui_out_;
  reader::PieceCounter pieces_;
  ILog* log_;
  std::optional<reader::Reader> reader_;
  // Title, not index: the library index can shift while the book is open (an upload may add a book).
  char title_[64] = "";
  std::uint32_t label_until_ms_ = 0;  // 0 means the label is not shown
};

}  // namespace leeloo::app
