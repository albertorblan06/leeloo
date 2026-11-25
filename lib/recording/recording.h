#pragma once

#include <cstddef>
#include <cstdint>

namespace leeloo::recording {

constexpr std::size_t kWavHeaderBytes = 44;
constexpr std::size_t kReserveBytes = 2097152;
constexpr std::size_t kWriteSlackBytes = 16384;
constexpr std::uint32_t kLongSampleRate = 8000;
constexpr std::uint32_t kLongSeconds = 120;
constexpr std::uint32_t kShortSampleRate = 16000;
constexpr std::uint32_t kShortSeconds = 30;

struct Format {
  std::uint32_t sample_rate = 0;
  std::uint32_t max_seconds = 0;
  std::uint32_t max_pcm_bytes = 0;

  std::uint32_t max_wav_bytes() const {
    return static_cast<std::uint32_t>(kWavHeaderBytes) + max_pcm_bytes;
  }
};

// Chooses the requested two-minute format when possible, then the tested
// 30-second format. Both choices preserve the filesystem reserve.
Format choose_format(std::size_t free_bytes);

void make_wav_header(std::uint8_t* out, std::uint32_t sample_rate, std::uint32_t pcm_bytes);
bool valid_wav_header(const std::uint8_t* header, std::size_t file_bytes);
std::int16_t sample_to_pcm16(std::int32_t sample);

}  // namespace leeloo::recording
