#include "recording.h"

#include <algorithm>
#include <cstring>
#include <limits>

namespace leeloo::recording {
namespace {

std::uint32_t pcm_bytes_for(std::uint32_t sample_rate, std::uint32_t seconds) {
  return sample_rate * seconds * 2;
}

void write_u16(std::uint8_t* out, std::uint16_t value) {
  out[0] = static_cast<std::uint8_t>(value);
  out[1] = static_cast<std::uint8_t>(value >> 8);
}

void write_u32(std::uint8_t* out, std::uint32_t value) {
  for (int i = 0; i < 4; ++i) out[i] = static_cast<std::uint8_t>(value >> (8 * i));
}

std::uint16_t read_u16(const std::uint8_t* in) {
  return static_cast<std::uint16_t>(in[0] | (in[1] << 8));
}

std::uint32_t read_u32(const std::uint8_t* in) {
  return static_cast<std::uint32_t>(in[0]) | (static_cast<std::uint32_t>(in[1]) << 8) |
         (static_cast<std::uint32_t>(in[2]) << 16) | (static_cast<std::uint32_t>(in[3]) << 24);
}

}  // namespace

Format plan_format(std::size_t free_bytes) {
  const std::size_t keep = kReserveBytes + kWriteSlackBytes + kWavHeaderBytes;
  if (free_bytes <= keep) return {};
  const std::size_t seconds = std::min<std::size_t>((free_bytes - keep) / kBytesPerSecond, kMaxSeconds);
  if (seconds < kMinSeconds) return {};
  const auto s = static_cast<std::uint32_t>(seconds);
  return {kSampleRate, s, pcm_bytes_for(kSampleRate, s)};
}

void make_wav_header(std::uint8_t* out, std::uint32_t sample_rate, std::uint32_t pcm_bytes) {
  std::memset(out, 0, kWavHeaderBytes);
  std::memcpy(out, "RIFF", 4);
  write_u32(out + 4, 36 + pcm_bytes);
  std::memcpy(out + 8, "WAVEfmt ", 8);
  write_u32(out + 16, 16);              // fmt chunk size
  write_u16(out + 20, 1);               // PCM
  write_u16(out + 22, 1);               // mono
  write_u32(out + 24, sample_rate);
  write_u32(out + 28, sample_rate * 2);  // bytes per second
  write_u16(out + 32, 2);               // block align
  write_u16(out + 34, 16);              // bits per sample
  std::memcpy(out + 36, "data", 4);
  write_u32(out + 40, pcm_bytes);
}

bool valid_wav_header(const std::uint8_t* h, std::size_t file_bytes) {
  if (file_bytes < kWavHeaderBytes || file_bytes > std::numeric_limits<std::uint32_t>::max()) {
    return false;
  }
  const std::uint32_t rate = read_u32(h + 24);
  std::uint32_t max_pcm = 0;
  if (rate == kSampleRate || rate == kLegacySampleRate) max_pcm = pcm_bytes_for(rate, kMaxSeconds);
  const std::uint32_t pcm = read_u32(h + 40);
  return max_pcm != 0 && std::memcmp(h, "RIFF", 4) == 0 && std::memcmp(h + 8, "WAVEfmt ", 8) == 0 &&
         std::memcmp(h + 36, "data", 4) == 0 && read_u32(h + 16) == 16 && read_u16(h + 20) == 1 &&
         read_u16(h + 22) == 1 && read_u32(h + 28) == rate * 2 && read_u16(h + 32) == 2 &&
         read_u16(h + 34) == 16 && pcm > 0 && pcm <= max_pcm && pcm % 2 == 0 &&
         file_bytes == kWavHeaderBytes + pcm && read_u32(h + 4) == file_bytes - 8;
}

std::int16_t sample_to_pcm16(std::int32_t sample) {
  const std::int32_t pcm = sample / 65536;  // division, not a shift: defined for negatives
  return static_cast<std::int16_t>(std::clamp<std::int32_t>(pcm, -32768, 32767));
}

std::uint32_t bytes_to_write(std::uint32_t written, std::uint32_t max_pcm_bytes,
                             std::uint32_t offered) {
  if (written >= max_pcm_bytes) return 0;
  return std::min(offered, max_pcm_bytes - written) & ~1u;
}

bool pause_expired(std::uint32_t paused_at_ms, std::uint32_t now_ms) {
  return static_cast<std::uint32_t>(now_ms - paused_at_ms) >= kMaxPauseMs;
}

}  // namespace leeloo::recording
