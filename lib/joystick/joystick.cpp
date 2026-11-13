#include "joystick.h"

namespace leeloo::joystick {

Direction Axis::update(int value) {
  const int delta = value - center_;

  if (delta >= kEnterDelta && state_ != Direction::High) {
    state_ = Direction::High;
    return Direction::High;
  }
  if (delta <= -kEnterDelta && state_ != Direction::Low) {
    state_ = Direction::Low;
    return Direction::Low;
  }
  // Inside the exit band the stick counts as centered again. Being exactly on
  // the boundary should also re-arm the axis, so the center threshold is
  // inclusive.
  if (delta >= -kExitDelta && delta <= kExitDelta) state_ = Direction::None;
  return Direction::None;
}

}  // namespace leeloo::joystick
