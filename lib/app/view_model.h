#pragma once

#include <cstdint>

#include "reading_frame.h"
#include "rtos.h"
#include "settings.h"
#include "ui.h"

namespace leeloo::app {

// What the Upload screen shows about the AP. A copy of the PIN, never a pointer
// into the server, so the AP can stop while a frame is being drawn.
struct NetStatus {
  bool running = false;
  char pin[16] = {};
};

// Everything the display task needs for one frame.
struct Snapshot {
  ui::Ui ui{settings::Settings{}};
  NetStatus net;
  ReadingFrame frame;
  // Contrast is written to the panel by the display task only, since it owns the I2C bus.
  bool contrast_pending = false;
  std::uint8_t contrast = 0;
};

// State shared by the tasks that decide what to show (UI, RSVP, net) and the one
// that draws it. Each producer updates its own part under a mutex and wakes the
// display task; the display task reads a copy. Latest wins: if the display is
// slower than the producers, intermediate states are skipped, never queued.
class ViewModel {
 public:
  ViewModel(rtos::IMutex* mutex, rtos::INotifier* display);

  void publish_ui(const ui::Ui& ui);
  void publish_frame(const ReadingFrame& frame);
  void publish_net(const NetStatus& net);
  void request_contrast(std::uint8_t register_value);

  // Copies the current state into `out` when something changed since the caller's
  // last read (`*seen` holds the version it saw). A pending contrast request is
  // handed over once. Returns false when there is nothing new.
  bool read_if_new(Snapshot* out, std::uint32_t* seen);

 private:
  template <typename Fn>
  void update(Fn&& apply) {
    {
      rtos::ScopedLock lock(mutex_);
      apply(state_);
      ++version_;
    }
    display_->notify();  // outside the lock, so the woken task does not run into it at once
  }

  rtos::IMutex* mutex_;
  rtos::INotifier* display_;
  Snapshot state_;
  std::uint32_t version_ = 0;
};

}  // namespace leeloo::app
