#pragma once

namespace leeloo::joystick {

// Distance from the center (12-bit ADC counts) needed to register a push, and
// the smaller distance the stick must return within before it re-arms.
// Initial values, not measured on hardware.
constexpr int kEnterDelta = 800;
constexpr int kExitDelta = 500;

enum class Direction { None, Low, High };

// One analog axis turned into discrete pushes with a dead zone and hysteresis.
// Emits a direction once when the stick enters it; no repeat while held.
class Axis {
 public:
  explicit Axis(int center = 2048) : center_(center) {}

  Direction update(int value);

 private:
  int center_;
  Direction state_ = Direction::None;
};

}  // namespace leeloo::joystick
