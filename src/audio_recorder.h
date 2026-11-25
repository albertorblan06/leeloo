#pragma once

#include <FS.h>
#include <cstdint>

#include "messages.h"
#include "ports.h"
#include "recording_frame.h"
#include "rtos.h"
#include "view_model.h"
#include "ui.h"

namespace leeloo::audio_recorder {

class Recorder {
 public:
  Recorder(rtos::Queue<ui::Event>* ui_out, app::ViewModel* view, app::ILog* log);

  void begin();
  void handle(const app::RecordingMsg& message);
  void poll();
  std::uint32_t poll_interval_ms() const;

 private:
  bool start();
  bool init_i2s(std::uint32_t sample_rate);
  void stop_i2s();
  void finish();
  void fail(ui::Event event, const char* log_line);
  void post(ui::Event event, const char* failure_line);
  void publish();
  void release_mutation();

  rtos::Queue<ui::Event>* ui_out_;
  app::ViewModel* view_;
  app::ILog* log_;
  app::RecordingFrame frame_;
  File file_;
  std::uint32_t pcm_bytes_ = 0;
  std::uint32_t last_publish_ms_ = 0;
  bool i2s_installed_ = false;
  bool running_ = false;
  bool mutation_held_ = false;
};

}  // namespace leeloo::audio_recorder
