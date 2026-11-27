#pragma once

#include <FS.h>

#include <cstddef>
#include <cstdint>

#include "ports.h"

// Device implementations of IMic and IRecordingStore (lib/app/ports.h). They move bytes and
// nothing else: the capture rules live in lib/app/recording_service.cpp.
// Covered by docs/hardware-checklists/audio-recording.md.
namespace leeloo::adapters {

// INMP441 on I2S port 0: BCLK GPIO15, WS GPIO16, data GPIO17, L/R to GND (left channel).
class I2sMic : public app::IMic {
 public:
  bool start(std::uint32_t sample_rate) override;
  void stop() override;
  bool pause() override;
  bool resume() override;
  int read(std::int16_t* out, std::size_t max) override;

 private:
  bool installed_ = false;
};

class LittleFsRecordingStore : public app::IRecordingStore {
 public:
  bool ready() override;
  bool exists() override;
  std::size_t free_bytes() override;
  bool lock() override;
  void unlock() override;
  bool open_temp(std::uint32_t sample_rate) override;
  bool write(const std::uint8_t* data, std::size_t size) override;
  bool finish(std::uint32_t sample_rate, std::uint32_t pcm_bytes) override;
  void abort() override;
  bool remove() override;
  void cleanup_temp() override;

 private:
  File file_;
  bool locked_ = false;
};

}  // namespace leeloo::adapters
