#pragma once

#include <cstdint>

// The boot animation of Loudop (low-dopamine-learn: web/src/components/BootSplash.tsx), adapted to
// a 128x64 1-bit panel. A dot with a fixed guide above it and a second guide, the "hand", that
// starts stacked on the first one and turns clockwise through the right side to hang below the dot.
// The word types out one letter every 110 ms, then the dot beats. Faithful to Loudop: the mark,
// the easing curve, the 180 degree turn, the 110 ms typing and the beat keyframes. Adapted: Loudop
// turns the hand in steps that follow real loading progress (450 ms each), here it is one eased
// 900 ms turn on a fixed timeline; Loudop scales the whole logo in the beat, here only the dot
// (at 8 px the guides would not visibly change); the word sits to the right of the mark, not
// below it, because the panel is only 64 px high. Pure timing and geometry: the drawing is in
// src/display_ui.cpp.
namespace leeloo::boot_anim {

constexpr const char* kWord = "leeloo";  // lowercase, as Loudop writes its name
constexpr int kWordLetters = 6;

constexpr std::uint32_t kSweepMs = 900;      // the hand turns 0 -> 180 degrees
constexpr std::uint32_t kLetterMs = 110;     // one letter every 110 ms (660 ms in all)
constexpr std::uint32_t kBeatStartMs = 900;  // the dot beats once the turn and the word are done
constexpr std::uint32_t kBeatMs = 340;
constexpr std::uint32_t kTotalMs = 1900;     // the logo then rests so that it can be read

// Geometry at 1:1 on the 64 px high panel (Loudop's 64-unit box).
constexpr int kDotRadius = 8;
constexpr int kHandInner = 12;  // the guides span radius 12..23 from the centre of the dot
constexpr int kHandOuter = 23;

struct Frame {
  int hand_degrees;  // 0 = up, on top of the fixed guide; 180 = down
  int letters;       // how many letters of kWord are visible
  int dot_permille;  // dot size, 1000 = nominal
  bool done;         // the animation has finished: the last frame stays
};

// The frame to show `elapsed_ms` after the animation began. Past kTotalMs it is the final frame.
Frame frame_at(std::uint32_t elapsed_ms);

struct Offset {
  int x;
  int y;
};

// Point at `radius` from the centre of the dot, `degrees` clockwise from straight up. Screen
// coordinates: y grows downwards.
Offset polar(int degrees, int radius);

}  // namespace leeloo::boot_anim
