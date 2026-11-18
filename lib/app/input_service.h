#pragma once

#include <cstdint>

#include "buttons.h"
#include "messages.h"
#include "ports.h"
#include "rtos.h"
#include "ui.h"

namespace leeloo::app {

// Turns raw button edges into menu events: debounce, short/long classification.
// The task blocks for wait_ms() on the edge queue, passes each edge to on_edge()
// and calls on_timeout() when the wait runs out.
class InputService {
 public:
  InputService(IPins* pins, rtos::Queue<ui::Event>* ui_out, ILog* log = nullptr);

  // Waits for debounce or the next held-button repeat; forever while idle.
  std::uint32_t wait_ms() const;

  void on_edge(const RawEdge& edge);
  void on_timeout(std::uint32_t now_ms);

 private:
  struct ButtonState {
    buttons::Debouncer debouncer;
    std::uint32_t pressed_at = 0;
    std::uint32_t next_repeat_at = 0;
    bool repeating = false;
  };

  void handle_level(std::uint8_t button, bool high, std::uint32_t now_ms);

  IPins* pins_;
  rtos::Queue<ui::Event>* ui_out_;
  ILog* log_;
  ButtonState states_[kButtonCount];
  bool settling_ = false;
  std::uint32_t now_ms_ = 0;
};

}  // namespace leeloo::app
