#include "leeloo_core.h"

namespace leeloo {

int utf8_sequence_length(std::uint8_t lead) {
  if (lead < 0x80) return 1;
  if (lead >= 0xC2 && lead <= 0xDF) return 2;  // 0xC0/0xC1 are overlong
  if (lead >= 0xE0 && lead <= 0xEF) return 3;
  if (lead >= 0xF0 && lead <= 0xF4) return 4;  // above 0xF4 exceeds U+10FFFF
  return 0;
}

}  // namespace leeloo
