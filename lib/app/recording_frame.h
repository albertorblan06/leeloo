#pragma once

#include <cstdint>

namespace leeloo::app {

// What the display shows about the recorder. Plain values, copied to the display task.
struct RecordingFrame {
  bool active = false;
  bool paused = false;
  std::uint32_t elapsed_seconds = 0;
  std::uint32_t max_seconds = 0;
  std::uint32_t sample_rate = 0;
};

}  // namespace leeloo::app
