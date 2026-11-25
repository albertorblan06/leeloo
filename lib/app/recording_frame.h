#pragma once

#include <cstdint>

namespace leeloo::app {

struct RecordingFrame {
  bool active = false;
  bool paused = false;
  bool available = false;
  std::uint32_t elapsed_seconds = 0;
  std::uint32_t max_seconds = 0;
  std::uint32_t sample_rate = 0;
};

}  // namespace leeloo::app
