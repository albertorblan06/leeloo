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

  // kWaitForever while idle. While a change settles it is one debounce window, after
  // which on_timeout() samples the pins so the debouncer can confirm the level.
  std::uint32_t wait_ms() const { return settling_ ? buttons::kDebounceMs : rtos::kWaitForever; }

  void on_edge(const RawEdge& edge);
  void on_timeout(std::uint32_t now_ms);

 private:
  struct ButtonState {
    buttons::Debouncer debouncer;
    std::uint32_t pressed_at = 0;
  };

  void handle_level(std::uint8_t button, bool high, std::uint32_t now_ms);

  IPins* pins_;
  rtos::Queue<ui::Event>* ui_out_;
  ILog* log_;
  ButtonState states_[kButtonCount];
  bool settling_ = false;
};

}  // namespace leeloo::app
