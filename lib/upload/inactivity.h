#pragma once

#include <cstdint>

namespace leeloo::upload {

// Auto-mode upload session ends after this much time without upload activity
// (docs/DEVICE-MENU.md §2 Biblioteca). A design default to validate on hardware.
constexpr std::uint32_t kUploadInactivityMs = 5u * 60u * 1000u;

// Tracks upload activity against a limit. Unsigned subtraction keeps the window
// correct across the 32-bit millis() wrap.
class InactivityTimer {
 public:
  explicit InactivityTimer(std::uint32_t limit_ms) : limit_ms_(limit_ms) {}

  void start(std::uint32_t now_ms) {
    last_ms_ = now_ms;
    running_ = true;
  }
  void touch(std::uint32_t now_ms) { last_ms_ = now_ms; }
  void stop() { running_ = false; }

  bool expired(std::uint32_t now_ms) const {
    return running_ && static_cast<std::uint32_t>(now_ms - last_ms_) >= limit_ms_;
  }

 private:
  std::uint32_t limit_ms_;
  std::uint32_t last_ms_ = 0;
  bool running_ = false;
};

}  // namespace leeloo::upload
