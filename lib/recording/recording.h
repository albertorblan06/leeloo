#pragma once

#include <cstddef>
#include <cstdint>

// Pure logic of the audio recording (docs/DEVICE-MENU.md "Grabar"): what format fits,
// the WAV header, how much of a chunk may still be written. No I2S or filesystem here.
namespace leeloo::recording {

constexpr std::size_t kWavHeaderBytes = 44;
// Free space that is never used by a recording, so books and settings keep room.
constexpr std::size_t kReserveBytes = 2097152;
constexpr std::size_t kWriteSlackBytes = 16384;  // extra headroom for filesystem metadata

// Always 16 kHz mono 16-bit: the rate speech recognition works best with. The length is
// whatever the free space allows, between kMinSeconds and kMaxSeconds. WAVs written by an
// earlier firmware at 8 kHz are still accepted by valid_wav_header.
constexpr std::uint32_t kSampleRate = 16000;
constexpr std::uint32_t kBytesPerSecond = kSampleRate * 2;
constexpr std::uint32_t kMinSeconds = 5;    // below this a recording is not worth starting
constexpr std::uint32_t kMaxSeconds = 600;  // 19.2 MB: under the 25 MB a speech API accepts
constexpr std::uint32_t kLegacySampleRate = 8000;

// A paused recording keeps the storage reserved, so it is saved after this long.
constexpr std::uint32_t kMaxPauseMs = 5 * 60 * 1000;

struct Format {
  std::uint32_t sample_rate = 0;  // 0 means nothing fits
  std::uint32_t max_seconds = 0;
  std::uint32_t max_pcm_bytes = 0;

  std::uint32_t max_wav_bytes() const {
    return static_cast<std::uint32_t>(kWavHeaderBytes) + max_pcm_bytes;
  }
};

// How a recording fits the free space: 16 kHz and as many seconds as remain after the
// reserve, the write margin and the header, capped at kMaxSeconds. A Format with sample_rate 0
// when fewer than kMinSeconds fit.
Format plan_format(std::size_t free_bytes);

// Writes the 44-byte header of a mono 16-bit PCM WAV into `out`.
void make_wav_header(std::uint8_t* out, std::uint32_t sample_rate, std::uint32_t pcm_bytes);

// True when `header` (the first 44 bytes) describes exactly a file of `file_bytes` that
// this firmware could have written: known rate, size within that format's limit.
bool valid_wav_header(const std::uint8_t* header, std::size_t file_bytes);

// 24 bits left-justified in a 32-bit slot (INMP441) to 16-bit PCM.
std::int16_t sample_to_pcm16(std::int32_t sample);

// How many of `offered` bytes may be written before the file reaches `max_pcm_bytes`.
// Whole 16-bit samples only, so the last chunk is cut instead of overshooting the limit.
std::uint32_t bytes_to_write(std::uint32_t written, std::uint32_t max_pcm_bytes,
                             std::uint32_t offered);

// True once a recording has been paused for kMaxPauseMs. Safe across a millis() wrap.
bool pause_expired(std::uint32_t paused_at_ms, std::uint32_t now_ms);

}  // namespace leeloo::recording
