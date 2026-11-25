#include "recording.h"

#include <algorithm>
#include <cstring>
#include <limits>

namespace leeloo::recording {
namespace {

std::uint32_t pcm_bytes(std::uint32_t sample_rate, std::uint32_t seconds) {
  return sample_rate * seconds * 2;
}

void write_u16(std::uint8_t* out, std::uint16_t value) {
  out[0] = static_cast<std::uint8_t>(value);
  out[1] = static_cast<std::uint8_t>(value >> 8);
}

void write_u32(std::uint8_t* out, std::uint32_t value) {
  out[0] = static_cast<std::uint8_t>(value);
  out[1] = static_cast<std::uint8_t>(value >> 8);
  out[2] = static_cast<std::uint8_t>(value >> 16);
  out[3] = static_cast<std::uint8_t>(value >> 24);
}

std::uint16_t read_u16(const std::uint8_t* in) {
  return static_cast<std::uint16_t>(in[0]) |
         (static_cast<std::uint16_t>(in[1]) << 8);
}

std::uint32_t read_u32(const std::uint8_t* in) {
  return static_cast<std::uint32_t>(in[0]) |
         (static_cast<std::uint32_t>(in[1]) << 8) |
         (static_cast<std::uint32_t>(in[2]) << 16) |
         (static_cast<std::uint32_t>(in[3]) << 24);
}

}  // namespace

Format choose_format(std::size_t free_bytes) {
  const Format long_format{kLongSampleRate, kLongSeconds,
                           pcm_bytes(kLongSampleRate, kLongSeconds)};
  const Format short_format{kShortSampleRate, kShortSeconds,
                            pcm_bytes(kShortSampleRate, kShortSeconds)};
  const std::size_t reserve = kReserveBytes + kWriteSlackBytes;
  if (free_bytes >= reserve + long_format.max_wav_bytes()) return long_format;
  if (free_bytes >= reserve + short_format.max_wav_bytes()) return short_format;
  return {};
}

void make_wav_header(std::uint8_t* out, std::uint32_t sample_rate, std::uint32_t pcm_size) {
  std::memset(out, 0, kWavHeaderBytes);
  std::memcpy(out, "RIFF", 4);
  write_u32(out + 4, 36 + pcm_size);
  std::memcpy(out + 8, "WAVEfmt ", 8);
  write_u32(out + 16, 16);
  write_u16(out + 20, 1);
  write_u16(out + 22, 1);
  write_u32(out + 24, sample_rate);
  write_u32(out + 28, sample_rate * 2);
  write_u16(out + 32, 2);
  write_u16(out + 34, 16);
  std::memcpy(out + 36, "data", 4);
  write_u32(out + 40, pcm_size);
}

bool valid_wav_header(const std::uint8_t* h, std::size_t file_bytes) {
  if (file_bytes < kWavHeaderBytes || file_bytes > std::numeric_limits<std::uint32_t>::max()) {
    return false;
  }
  const std::uint32_t rate = read_u32(h + 24);
  const std::uint32_t pcm_size = read_u32(h + 40);
  const std::uint32_t max_pcm = rate == kLongSampleRate
                                    ? pcm_bytes(kLongSampleRate, kLongSeconds)
                                    : rate == kShortSampleRate
                                          ? pcm_bytes(kShortSampleRate, kShortSeconds)
                                          : 0;
  return std::memcmp(h, "RIFF", 4) == 0 &&
         std::memcmp(h + 8, "WAVEfmt ", 8) == 0 &&
         std::memcmp(h + 36, "data", 4) == 0 &&
         read_u32(h + 16) == 16 &&
         read_u16(h + 20) == 1 &&
         read_u16(h + 22) == 1 &&
         (rate == kLongSampleRate || rate == kShortSampleRate) &&
         read_u32(h + 28) == rate * 2 &&
         read_u16(h + 32) == 2 &&
         read_u16(h + 34) == 16 &&
         pcm_size > 0 && pcm_size <= max_pcm && pcm_size % 2 == 0 &&
         file_bytes == kWavHeaderBytes + pcm_size &&
         read_u32(h + 4) == file_bytes - 8;
}

std::int16_t sample_to_pcm16(std::int32_t sample) {
  const std::int32_t shifted = sample / 65536;
  return static_cast<std::int16_t>(std::max<std::int32_t>(
      -32768, std::min<std::int32_t>(32767, shifted)));
}

}  // namespace leeloo::recording
