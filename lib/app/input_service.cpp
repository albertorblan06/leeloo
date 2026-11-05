#include "input_service.h"

namespace leeloo::app {

namespace {

constexpr const char* kButtonNames[kButtonCount] = {"ARRIBA", "ABAJO", "SELECT"};

}  // namespace

InputService::InputService(IPins* pins, rtos::Queue<ui::Event>* ui_out, ILog* log)
    : pins_(pins), ui_out_(ui_out), log_(log) {}

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
  } else if (edge == buttons::Edge::Released && button == kSelect) {
    const buttons::Press kind = buttons::classify(buttons::elapsed_ms(state.pressed_at, now_ms));
    post(ui_out_, kind == buttons::Press::Long ? ui::Event::Back : ui::Event::Select, log_,
         "Cola de UI llena: se pierde SELECT");
  }
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
}

}  // namespace leeloo::app
