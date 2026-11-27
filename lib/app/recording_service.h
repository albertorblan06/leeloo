#pragma once

#include <cstdint>

#include "messages.h"
#include "ports.h"
#include "recording.h"
#include "recording_frame.h"
#include "rtos.h"
#include "ui.h"

namespace leeloo::app {

// Owns one capture at a time: format choice, writing, pause, limits and finishing. UiService
// sends commands; this service answers on the UI queue (AudioSaved, AudioNoSpace, ...) so
// the screen always follows what really happened. The recording task blocks for wait_ms(),
// calls handle() for each command and poll() when the wait runs out.
class RecordingService {
 public:
  RecordingService(IMic* mic, IRecordingStore* store, rtos::Queue<ui::Event>* ui_out,
                   ILog* log = nullptr);

  // Once, when the task starts: drops temporary leftovers and reports what is stored.
  void begin();

  // Both return true when the frame changed and must be published again.
  bool handle(const RecordingMsg& msg, std::uint32_t now_ms);
  bool poll(std::uint32_t now_ms);

  // kWaitForever when idle or paused with nothing to expire; otherwise the time to the next poll.
  std::uint32_t wait_ms(std::uint32_t now_ms) const;

  RecordingFrame frame() const { return frame_; }

 private:
  static constexpr std::uint32_t kPollMs = 20;  // I2S buffers hold far more than this
  static constexpr std::uint32_t kPausedPollMs = 1000;
  static constexpr std::size_t kReadSamples = 256;
  // Reads per poll. 32 x 256 samples is half a second at 16 kHz: enough to catch up after a
  // slow flash write, and bounded so the task still feeds the watchdog and sees commands.
  static constexpr int kMaxReadsPerPoll = 32;

  void start();
  void toggle_pause(std::uint32_t now_ms);
  void finish();
  void fail(ui::Event event, const char* what);
  void stop_after_error(ui::Event event, const char* what);
  void send(ui::Event event);
  void release();
  bool write_samples(const std::int16_t* samples, std::size_t count);

  IMic* mic_;
  IRecordingStore* store_;
  rtos::Queue<ui::Event>* ui_out_;
  ILog* log_;
  RecordingFrame frame_;
  recording::Format format_;
  bool running_ = false;
  bool locked_ = false;
  bool backlog_ = false;  // the last poll stopped at its read limit with data still buffered
  std::uint32_t pcm_bytes_ = 0;
  std::uint32_t paused_at_ms_ = 0;
};

}  // namespace leeloo::app
