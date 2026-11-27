#include "boot_anim.h"

#include <cmath>

namespace leeloo::boot_anim {
namespace {

// cubic-bezier(.22, 1, .36, 1), Loudop's --ease: output at x = i / 16, in permille. Fast at
// first, then a long gentle finish.
constexpr int kEase[17] = {0,   264, 482, 647, 765, 846, 901, 937, 961,
                           977, 987, 993, 997, 999, 1000, 1000, 1000};

// Progress 0..1000 of the sweep after `ms`, eased.
int eased_permille(std::uint32_t ms) {
  if (ms >= kSweepMs) return 1000;
  const std::uint32_t scaled = ms * 16 * 1000 / kSweepMs;  // position in 1/1000 of a table step
  const std::uint32_t index = scaled / 1000;
  const std::uint32_t rest = scaled % 1000;
  return kEase[index] + static_cast<int>((kEase[index + 1] - kEase[index]) * rest / 1000);
}

// The @keyframes boot-beat of Loudop: 0 % 1, 28 % 1.13, 55 % 0.98, 78 % 1.07, 100 % 1.
struct Key {
  std::uint32_t percent;
  int permille;
};
constexpr Key kBeat[] = {{0, 1000}, {28, 1130}, {55, 980}, {78, 1070}, {100, 1000}};

int beat_permille(std::uint32_t ms) {
  if (ms >= kBeatMs) return 1000;
  const std::uint32_t at = ms * 100 * 1000 / kBeatMs;  // percent of the beat, in thousandths
  for (int i = 1; i < 5; ++i) {
    const std::uint32_t end = kBeat[i].percent * 1000;
    if (at > end) continue;
    const std::uint32_t start = kBeat[i - 1].percent * 1000;
    const int from = kBeat[i - 1].permille;
    const int to = kBeat[i].permille;
    return from + static_cast<int>((to - from) * static_cast<std::int64_t>(at - start) / static_cast<std::int64_t>(end - start));
  }
  return 1000;
}

}  // namespace

Frame frame_at(std::uint32_t elapsed_ms) {
  Frame f;
  f.hand_degrees = eased_permille(elapsed_ms) * 180 / 1000;
  const std::uint32_t typed = elapsed_ms / kLetterMs;
  f.letters = typed > static_cast<std::uint32_t>(kWordLetters) ? kWordLetters : static_cast<int>(typed);
  f.dot_permille = elapsed_ms >= kBeatStartMs ? beat_permille(elapsed_ms - kBeatStartMs) : 1000;
  f.done = elapsed_ms >= kTotalMs;
  return f;
}

Offset polar(int degrees, int radius) {
  const double radians = degrees * 3.14159265358979323846 / 180.0;
  return {static_cast<int>(std::lround(radius * std::sin(radians))),
          static_cast<int>(std::lround(-radius * std::cos(radians)))};
}

}  // namespace leeloo::boot_anim
