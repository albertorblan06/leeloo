#include "buttons.h"

namespace leeloo::buttons {

Edge Debouncer::update(bool high, std::uint32_t now_ms) {
  if (high != raw_high_) {
    raw_high_ = high;
    changed_at_ = now_ms;
    return Edge::None;
  }
  if (elapsed_ms(changed_at_, now_ms) < kDebounceMs) return Edge::None;
  if (high == stable_high_) return Edge::None;
  stable_high_ = high;
  return high ? Edge::Released : Edge::Pressed;
}

std::uint32_t elapsed_ms(std::uint32_t from, std::uint32_t to) { return to - from; }

Press classify(std::uint32_t press_ms) {
  return press_ms >= kLongPressMs ? Press::Long : Press::Short;
}

}  // namespace leeloo::buttons
