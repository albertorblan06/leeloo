#pragma once

#include <cstdint>

namespace leeloo {

// Returns the byte length (1-4) of the UTF-8 sequence introduced by `lead`,
// or 0 if `lead` cannot start a sequence (continuation or invalid byte).
int utf8_sequence_length(std::uint8_t lead);

}  // namespace leeloo
