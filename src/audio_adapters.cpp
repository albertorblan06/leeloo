#include "audio_adapters.h"

#include <Arduino.h>
#include <LittleFS.h>
#include <driver/i2s.h>

#include <algorithm>

#include "recording.h"
#include "storage_books.h"
#include "storage_recording.h"

namespace leeloo::adapters {
namespace {

constexpr i2s_port_t kI2sPort = I2S_NUM_0;
constexpr int kI2sBclk = 15;
constexpr int kI2sWs = 16;
constexpr int kI2sData = 17;
constexpr std::size_t kReadSamples = 256;
// LittleFS can stall a write for tens of milliseconds while it erases a block. 16 buffers of
// 256 frames hold 512 ms at 8 kHz and 256 ms at 16 kHz, which rides that out. Internal RAM: 16 KiB.
constexpr int kDmaBuffers = 16;

}  // namespace

// --- Microphone -------------------------------------------------------------------

bool I2sMic::start(std::uint32_t sample_rate) {
  if (installed_) stop();
  i2s_config_t config = {};
  config.mode = static_cast<i2s_mode_t>(I2S_MODE_MASTER | I2S_MODE_RX);
  config.sample_rate = sample_rate;
  // The INMP441 sends 24 bits in a 32-bit slot, on the left channel when L/R is grounded.
  config.bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT;
  config.channel_format = I2S_CHANNEL_FMT_ONLY_LEFT;
  config.communication_format = I2S_COMM_FORMAT_STAND_I2S;
  config.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;
  config.dma_buf_count = kDmaBuffers;
  config.dma_buf_len = kReadSamples;
  if (i2s_driver_install(kI2sPort, &config, 0, nullptr) != ESP_OK) return false;
  installed_ = true;

  i2s_pin_config_t pins = {};
  pins.mck_io_num = I2S_PIN_NO_CHANGE;
  pins.bck_io_num = kI2sBclk;
  pins.ws_io_num = kI2sWs;
  pins.data_out_num = I2S_PIN_NO_CHANGE;
  pins.data_in_num = kI2sData;
  if (i2s_set_pin(kI2sPort, &pins) != ESP_OK) {
    stop();
    return false;
  }
  return true;  // the driver starts capturing as soon as it is installed
}

void I2sMic::stop() {
  if (!installed_) return;
  i2s_stop(kI2sPort);
  i2s_driver_uninstall(kI2sPort);
  installed_ = false;
}

bool I2sMic::pause() { return installed_ && i2s_stop(kI2sPort) == ESP_OK; }
bool I2sMic::resume() { return installed_ && i2s_start(kI2sPort) == ESP_OK; }

int I2sMic::read(std::int16_t* out, std::size_t max) {
  if (!installed_) return -1;
  std::int32_t raw[kReadSamples];
  const std::size_t wanted = std::min(max, kReadSamples);
  std::size_t bytes = 0;
  const esp_err_t result =
      i2s_read(kI2sPort, raw, wanted * sizeof raw[0], &bytes, pdMS_TO_TICKS(20));
  if (result == ESP_ERR_TIMEOUT) return 0;
  if (result != ESP_OK) return -1;
  const std::size_t count = bytes / sizeof raw[0];
  for (std::size_t i = 0; i < count; ++i) out[i] = recording::sample_to_pcm16(raw[i]);
  return static_cast<int>(count);
}

// --- Recording file ---------------------------------------------------------------

bool LittleFsRecordingStore::ready() { return storage::mounted(); }

bool LittleFsRecordingStore::exists() { return storage_recording::exists(); }

std::size_t LittleFsRecordingStore::free_bytes() { return storage::free_bytes(); }

bool LittleFsRecordingStore::lock() {
  if (locked_ || !storage::try_begin_mutation()) return false;
  locked_ = true;
  return true;
}

void LittleFsRecordingStore::unlock() {
  if (!locked_) return;
  storage::end_mutation();
  locked_ = false;
}

bool LittleFsRecordingStore::open_temp(std::uint32_t sample_rate) {
  storage::Guard guard;
  LittleFS.remove(storage_recording::kRecordingTempPath);
  file_ = LittleFS.open(storage_recording::kRecordingTempPath, "w");
  if (!file_) return false;
  std::uint8_t header[recording::kWavHeaderBytes];
  recording::make_wav_header(header, sample_rate, 0);
  if (file_.write(header, sizeof header) == sizeof header) return true;
  file_.close();
  LittleFS.remove(storage_recording::kRecordingTempPath);
  return false;
}

bool LittleFsRecordingStore::write(const std::uint8_t* data, std::size_t size) {
  storage::Guard guard;
  return file_ && file_.write(data, size) == size;
}

// Closes the temporary file, then trusts only what is on disk: the file is reopened, its
// length measured (a stat on an open write handle can lag behind buffered data), the header
// patched to match, and the result checked with valid_wav_header before it takes its final
// name. A write that failed half-way (disk full) can leave a few bytes more or fewer than
// were counted; a header that disagrees with the file would make the WAV unservable forever.
// A power cut before the rename leaves only the temp file, which boot removes.
bool LittleFsRecordingStore::finish(std::uint32_t sample_rate, std::uint32_t /*counted_pcm_bytes*/) {
  storage::Guard guard;
  if (!file_) return false;
  file_.flush();
  file_.close();

  File patch = LittleFS.open(storage_recording::kRecordingTempPath, "r+");
  if (!patch) return false;
  const std::size_t size = patch.size();
  if (size <= recording::kWavHeaderBytes) {  // no audio reached the disk
    patch.close();
    return false;
  }
  const auto pcm_bytes =
      static_cast<std::uint32_t>((size - recording::kWavHeaderBytes) & ~std::size_t{1});
  std::uint8_t header[recording::kWavHeaderBytes];
  recording::make_wav_header(header, sample_rate, pcm_bytes);
  const bool patched = patch.seek(0, SeekSet) && patch.write(header, sizeof header) == sizeof header;
  patch.flush();
  patch.close();
  if (!patched) return false;

  File check = LittleFS.open(storage_recording::kRecordingTempPath, "r");
  if (!check) return false;
  std::uint8_t stored[recording::kWavHeaderBytes];
  const bool valid = check.read(stored, sizeof stored) == sizeof stored &&
                     recording::valid_wav_header(stored, check.size());
  check.close();
  return valid && !LittleFS.exists(storage_recording::kRecordingPath) &&
         LittleFS.rename(storage_recording::kRecordingTempPath, storage_recording::kRecordingPath);
}

void LittleFsRecordingStore::abort() {
  storage::Guard guard;
  if (file_) file_.close();
  LittleFS.remove(storage_recording::kRecordingTempPath);
}

bool LittleFsRecordingStore::remove() {
  if (!storage::try_begin_mutation()) return false;  // a capture or an upload is using the files
  const bool removed = storage_recording::remove();
  storage::end_mutation();
  return removed;
}

void LittleFsRecordingStore::cleanup_temp() { storage_recording::cleanup_temp(); }

}  // namespace leeloo::adapters
