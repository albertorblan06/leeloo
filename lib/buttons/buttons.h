#pragma once

#include <cstdint>

namespace leeloo::buttons {

// Stable level must hold this long before a change counts (mechanical bounce).
// Initial value, not measured on hardware.
constexpr std::uint32_t kDebounceMs = 20;

// Press duration at or above this is a long press. Initial value, not measured.
constexpr std::uint32_t kLongPressMs = 800;

enum class Edge { None, Pressed, Released };
enum class Press { Short, Long };

// Debounces one active-low button. `high` is the pin level (true = released).
// All times use unsigned arithmetic, so millis() wrap-around is handled.
class Debouncer {
 public:
  Edge update(bool high, std::uint32_t now_ms);

  // True while a level change waits to be confirmed. The caller must keep
  // feeding samples until this turns false, or the change is lost.
  bool pending() const { return raw_high_ != stable_high_; }

 private:
  bool stable_high_ = true;  // last debounced level
  bool raw_high_ = true;     // last raw level seen
  std::uint32_t changed_at_ = 0;
};

// Milliseconds from `from` to `to`, correct across a 32-bit millis() wrap.
std::uint32_t elapsed_ms(std::uint32_t from, std::uint32_t to);

Press classify(std::uint32_t press_ms);

}  // namespace leeloo::buttons
