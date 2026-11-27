#include "input_service.h"

#include <algorithm>

namespace leeloo::app {

namespace {

constexpr const char* kButtonNames[kButtonCount] = {"ARRIBA", "ABAJO", "SELECT"};

// Milliseconds from `now` to `at`, 0 when already past. Correct across a 32-bit wrap.
std::uint32_t until(std::uint32_t at, std::uint32_t now) {
  const auto delta = static_cast<std::int32_t>(at - now);
  return delta <= 0 ? 0 : static_cast<std::uint32_t>(delta);
}

bool repeats(std::uint8_t button) { return button == kUp || button == kDown; }

}  // namespace

InputService::InputService(IPins* pins, rtos::Queue<ui::Event>* ui_out, ILog* log)
    : pins_(pins), ui_out_(ui_out), log_(log) {}

std::uint32_t InputService::wait_ms(std::uint32_t now_ms) const {
  std::uint32_t wait = settling_ ? buttons::kDebounceMs : rtos::kWaitForever;
  for (std::uint8_t b = 0; b < kButtonCount; ++b) {
    if (states_[b].repeating) wait = std::min(wait, until(states_[b].next_repeat_at, now_ms));
  }
  return wait;
}

// UP and DOWN act on the press, so scrolling is immediate. SELECT acts on the
// release, because its length decides between a short and a long press.
void InputService::handle_level(std::uint8_t button, bool high, std::uint32_t now_ms) {
  ButtonState& state = states_[button];
  const buttons::Edge edge = state.debouncer.update(high, now_ms);
  if (edge == buttons::Edge::Pressed) {
    state.pressed_at = now_ms;
    if (log_ != nullptr) log_->log(kButtonNames[button]);
    if (button == kUp) post(ui_out_, ui::Event::Up, log_, "Cola de UI llena: se pierde ARRIBA");
    if (button == kDown) post(ui_out_, ui::Event::Down, log_, "Cola de UI llena: se pierde ABAJO");
    if (repeats(button)) {
      state.repeating = true;
      state.next_repeat_at = now_ms + buttons::kRepeatDelayMs;
    }
  } else if (edge == buttons::Edge::Released) {
    state.repeating = false;
    if (button == kSelect) {
      const buttons::Press kind = buttons::classify(buttons::elapsed_ms(state.pressed_at, now_ms));
      post(ui_out_, kind == buttons::Press::Long ? ui::Event::Back : ui::Event::Select, log_,
           "Cola de UI llena: se pierde SELECT");
    }
  }
}

// One repeat per call, never a burst: after a late wake-up the schedule skips the
// missed beats instead of replaying them.
void InputService::repeat_held(std::uint8_t button, std::uint32_t now_ms) {
  ButtonState& state = states_[button];
  if (!state.repeating || static_cast<std::int32_t>(now_ms - state.next_repeat_at) < 0) return;
  // A release edge lost to a full edge queue must not leave the button repeating forever.
  if (pins_->read_high(button)) {
    state.repeating = false;
    return;
  }
  if (button == kUp) {
    post(ui_out_, ui::Event::UpRepeat, log_, "Cola de UI llena: se pierde repetición ARRIBA");
  } else {
    post(ui_out_, ui::Event::DownRepeat, log_, "Cola de UI llena: se pierde repetición ABAJO");
  }
  const std::uint32_t late = buttons::elapsed_ms(state.next_repeat_at, now_ms);
  state.next_repeat_at += (late / buttons::kRepeatIntervalMs + 1) * buttons::kRepeatIntervalMs;
}

void InputService::on_edge(const RawEdge& edge) {
  if (edge.button >= kButtonCount) return;  // a corrupt index must not reach the state array
  settling_ = true;
  handle_level(edge.button, edge.high, edge.ms);
}

void InputService::on_timeout(std::uint32_t now_ms) {
  settling_ = false;
  for (std::uint8_t b = 0; b < kButtonCount; ++b) {
    if (!states_[b].debouncer.pending()) continue;
    handle_level(b, pins_->read_high(b), now_ms);
    // The timeout can fire a tick before the window is over; keep sampling until
    // the change is confirmed or rejected, otherwise the press is lost.
    settling_ = settling_ || states_[b].debouncer.pending();
  }
  repeat_held(kUp, now_ms);
  repeat_held(kDown, now_ms);
}

}  // namespace leeloo::app
