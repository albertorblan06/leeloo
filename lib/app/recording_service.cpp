#include "recording_service.h"

#include <algorithm>

namespace leeloo::app {

RecordingService::RecordingService(IMic* mic, IRecordingStore* store,
                                   rtos::Queue<ui::Event>* ui_out, ILog* log)
    : mic_(mic), store_(store), ui_out_(ui_out), log_(log) {}

void RecordingService::send(ui::Event event) {
  post(ui_out_, event, log_, "Cola de UI llena: se pierde un aviso de audio");
}

void RecordingService::release() {
  if (!locked_) return;
  store_->unlock();
  locked_ = false;
}

void RecordingService::begin() {
  store_->cleanup_temp();
  if (!store_->ready()) {
    send(ui::Event::AudioStorageUnavailable);
    return;
  }
  if (store_->exists()) send(ui::Event::AudioAvailable);
}

bool RecordingService::handle(const RecordingMsg& msg, std::uint32_t now_ms) {
  switch (msg.cmd) {
    case RecordingCmd::Start:
      start();
      return true;
    case RecordingCmd::TogglePause:
      if (!running_) return false;
      toggle_pause(now_ms);
      return true;
    case RecordingCmd::Stop:
      // A Stop with nothing recording must still be answered, or the screen that asked for
      // it would wait for ever (the start may have failed and its message been dropped).
      if (!running_) {
        send(ui::Event::RecordingEnded);
        return false;
      }
      finish();
      return true;
    case RecordingCmd::Delete:
      if (running_) return false;  // the file being written is not the stored one
      if (store_->remove()) {
        send(ui::Event::AudioDeleted);
      } else {
        send(ui::Event::AudioDeleteFailed);
      }
      return false;
  }
  return false;
}

void RecordingService::start() {
  if (running_) return;
  if (!store_->lock()) {
    send(ui::Event::AudioBusy);
    return;
  }
  locked_ = true;
  if (store_->exists()) {
    release();
    if (log_ != nullptr) log_->log("Audio: ya hay una grabación pendiente");
    send(ui::Event::AudioStartFailed);
    return;
  }
  format_ = recording::plan_format(store_->free_bytes());
  if (format_.sample_rate == 0) {
    release();
    send(ui::Event::AudioNoSpace);
    return;
  }
  if (!store_->open_temp(format_.sample_rate)) {
    release();
    if (log_ != nullptr) log_->log("Audio: no se pudo crear el archivo temporal");
    send(ui::Event::AudioStartFailed);
    return;
  }
  if (!mic_->start(format_.sample_rate)) {
    store_->abort();
    release();
    if (log_ != nullptr) log_->log("Audio: no se pudo iniciar I2S");
    send(ui::Event::AudioStartFailed);
    return;
  }
  running_ = true;
  backlog_ = false;
  pcm_bytes_ = 0;
  frame_ = RecordingFrame{};
  frame_.active = true;
  frame_.max_seconds = format_.max_seconds;
  frame_.sample_rate = format_.sample_rate;
}

void RecordingService::toggle_pause(std::uint32_t now_ms) {
  const bool ok = frame_.paused ? mic_->resume() : mic_->pause();
  if (!ok) {
    stop_after_error(ui::Event::AudioSaveFailed, "Audio: no se pudo pausar o reanudar I2S");
    return;
  }
  frame_.paused = !frame_.paused;
  if (frame_.paused) paused_at_ms_ = now_ms;
}

// Closes the capture: stores it when it holds audio, discards it otherwise.
void RecordingService::finish() {
  mic_->stop();
  running_ = false;
  const std::uint32_t pcm = pcm_bytes_;
  const std::uint32_t rate = format_.sample_rate;
  frame_ = RecordingFrame{};
  if (pcm == 0) {
    store_->abort();
    release();
    if (log_ != nullptr) log_->log("Audio: grabación vacía descartada");
    send(ui::Event::AudioSaveFailed);
    return;
  }
  const bool saved = store_->finish(rate, pcm);
  if (!saved) store_->abort();
  release();
  if (saved) {
    send(ui::Event::AudioSaved);
  } else {
    if (log_ != nullptr) log_->log("Audio: no se pudo guardar el WAV");
    send(ui::Event::AudioSaveFailed);
  }
}

// A full disk, a lost microphone or a failed pause must not throw away minutes of recording:
// whatever was captured is saved. Only an empty capture is dropped as a failure.
void RecordingService::stop_after_error(ui::Event event, const char* what) {
  if (pcm_bytes_ == 0) {
    fail(event, what);
    return;
  }
  if (log_ != nullptr) log_->log(what);
  finish();
}

void RecordingService::fail(ui::Event event, const char* what) {
  mic_->stop();
  running_ = false;
  frame_ = RecordingFrame{};
  store_->abort();
  release();
  if (log_ != nullptr) log_->log(what);
  send(event);
}

bool RecordingService::write_samples(const std::int16_t* samples, std::size_t count) {
  std::uint8_t bytes[kReadSamples * 2];
  const std::uint32_t take =
      recording::bytes_to_write(pcm_bytes_, format_.max_pcm_bytes, static_cast<std::uint32_t>(count * 2));
  for (std::uint32_t i = 0; i < take / 2; ++i) {
    const auto value = static_cast<std::uint16_t>(samples[i]);
    bytes[2 * i] = static_cast<std::uint8_t>(value);  // little endian, whatever the CPU is
    bytes[2 * i + 1] = static_cast<std::uint8_t>(value >> 8);
  }
  if (take > 0 && !store_->write(bytes, take)) return false;
  pcm_bytes_ += take;
  return true;
}

bool RecordingService::poll(std::uint32_t now_ms) {
  if (!running_) return false;
  if (frame_.paused) {
    // A paused capture keeps the storage reserved, so it is saved rather than held for ever.
    if (!recording::pause_expired(paused_at_ms_, now_ms)) return false;
    finish();
    return true;
  }
  // One read returns at most kReadSamples, less than arrives between two wake-ups at 16 kHz,
  // so keep reading while the microphone has data. Otherwise the DMA buffer overflows and
  // the WAV silently loses audio while its byte count still looks complete.
  backlog_ = false;
  for (int reads = 0;; ++reads) {
    if (reads == kMaxReadsPerPoll) {
      backlog_ = true;
      break;
    }
    std::int16_t samples[kReadSamples];
    const int count = mic_->read(samples, kReadSamples);
    if (count < 0) {
      stop_after_error(ui::Event::AudioSaveFailed, "Audio: error al leer muestras I2S");
      return true;
    }
    if (count == 0) break;
    if (!write_samples(samples, static_cast<std::size_t>(count))) {
      stop_after_error(ui::Event::AudioSaveFailed, "Audio: fallo al escribir muestras (disco lleno)");
      return true;
    }
    if (pcm_bytes_ >= format_.max_pcm_bytes) {  // the limit ends the capture at the exact byte
      finish();
      return true;
    }
  }
  const std::uint32_t seconds = pcm_bytes_ / (format_.sample_rate * 2);
  if (seconds == frame_.elapsed_seconds) return false;
  frame_.elapsed_seconds = seconds;
  return true;
}

std::uint32_t RecordingService::wait_ms(std::uint32_t now_ms) const {
  if (!running_) return rtos::kWaitForever;
  if (!frame_.paused) return backlog_ ? 0 : kPollMs;
  const std::uint32_t since = static_cast<std::uint32_t>(now_ms - paused_at_ms_);
  const std::uint32_t left = since >= recording::kMaxPauseMs ? 0 : recording::kMaxPauseMs - since;
  return std::min(left, kPausedPollMs);
}

}  // namespace leeloo::app
