#pragma once

#include <cstddef>
#include <cstdint>

#include "messages.h"

// Three tactile buttons to GND with the internal pull-up (docs/hardware-checklists/buttons-input.md).
namespace leeloo::pins {

constexpr std::size_t kButtonCount = app::kButtonCount;
constexpr std::uint8_t kButtonPins[kButtonCount] = {5, 4, 6};  // UP, DOWN, SELECT, in app::Button order (UP/DOWN swapped to match the wiring)

}  // namespace leeloo::pins
