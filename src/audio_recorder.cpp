#include "audio_recorder.h"

#include <Arduino.h>
#include <LittleFS.h>
#include <driver/i2s.h>
#include <freertos/FreeRTOS.h>

#include <algorithm>
#include <cstdint>

#include "recording.h"
#include "storage_books.h"
#include "storage_recording.h"

namespace leeloo::audio_recorder {
namespace {

constexpr i2s_port_t kI2sPort = I2S_NUM_0;
constexpr std::uint8_t kI2sBclk = 15;
constexpr std::uint8_t kI2sWs = 16;
constexpr std::uint8_t kI2sData = 17;
constexpr std::size_t kReadSamples = 256;

}  // namespace

Recorder::Recorder(rtos::Queue<ui::Event>* ui_out, app::ViewModel* view, app::ILog* log)
    : ui_out_(ui_out), view_(view), log_(log) {}

void Recorder::begin() {
  storage_recording::cleanup_temp();
  if (!storage::mounted()) {
    post(ui::Event::AudioStorageUnavailable,
         "Cola de UI llena: se pierde AudioStorageUnavailable");
    return;
  }
  frame_.available = storage_recording::exists();
  publish();
  if (frame_.available) post(ui::Event::AudioAvailable, "Cola de UI llena: se pierde AudioAvailable");
}

void Recorder::handle(const app::RecordingMsg& message) {
  switch (message.cmd) {
    case app::RecordingCmd::Start:
      start();
      return;
    case app::RecordingCmd::Pause:
      if (!running_ || frame_.paused) return;
      if (i2s_stop(kI2sPort) != ESP_OK) {
        fail(ui::Event::AudioSaveFailed, "Audio: no se pudo pausar I2S");
        return;
      }
      frame_.paused = true;
      publish();
      post(ui::Event::RecordingPaused, "Cola de UI llena: se pierde RecordingPaused");
      return;
    case app::RecordingCmd::Resume:
      if (!running_ || !frame_.paused) return;
      if (i2s_start(kI2sPort) != ESP_OK) {
        fail(ui::Event::AudioSaveFailed, "Audio: no se pudo reanudar I2S");
        return;
      }
      frame_.paused = false;
      publish();
      post(ui::Event::RecordingResumed, "Cola de UI llena: se pierde RecordingResumed");
      return;
    case app::RecordingCmd::Stop:
      if (running_) finish();
      return;
    case app::RecordingCmd::Delete:
      if (running_) return;
      if (storage_recording::remove()) {
        frame_.available = false;
        publish();
        post(ui::Event::AudioDeleted, "Cola de UI llena: se pierde AudioDeleted");
      } else {
        post(ui::Event::AudioDeleteFailed, "Cola de UI llena: se pierde AudioDeleteFailed");
      }
      return;
  }
}

bool Recorder::init_i2s(std::uint32_t sample_rate) {
  i2s_config_t config = {};
  config.mode = static_cast<i2s_mode_t>(I2S_MODE_MASTER | I2S_MODE_RX);
  config.sample_rate = sample_rate;
  config.bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT;
  config.channel_format = I2S_CHANNEL_FMT_ONLY_LEFT;
  config.communication_format = I2S_COMM_FORMAT_STAND_I2S;
  config.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;
  config.dma_buf_count = 4;
  config.dma_buf_len = kReadSamples;
  config.use_apll = false;
  config.tx_desc_auto_clear = false;
  config.fixed_mclk = 0;
  if (i2s_driver_install(kI2sPort, &config, 0, nullptr) != ESP_OK) return false;
  i2s_installed_ = true;

  i2s_pin_config_t pins = {};
  pins.mck_io_num = I2S_PIN_NO_CHANGE;
  pins.bck_io_num = kI2sBclk;
  pins.ws_io_num = kI2sWs;
  pins.data_out_num = I2S_PIN_NO_CHANGE;
  pins.data_in_num = kI2sData;
  if (i2s_set_pin(kI2sPort, &pins) != ESP_OK || i2s_start(kI2sPort) != ESP_OK) {
    stop_i2s();
    return false;
  }
  return true;
}

void Recorder::stop_i2s() {
  if (!i2s_installed_) return;
  i2s_stop(kI2sPort);
  i2s_driver_uninstall(kI2sPort);
  i2s_installed_ = false;
}

bool Recorder::start() {
  if (running_) return true;
  if (!storage::try_begin_mutation()) {
    post(ui::Event::AudioBusy, "Audio: el almacenamiento está ocupado");
    return false;
  }
  mutation_held_ = true;
  if (storage_recording::exists()) {
    release_mutation();
    post(ui::Event::AudioStartFailed, "Audio: ya hay una grabación pendiente");
    return false;
  }

  const recording::Format format = recording::choose_format(storage::free_bytes());
  if (format.sample_rate == 0) {
    release_mutation();
    post(ui::Event::AudioNoSpace, "Audio: espacio insuficiente para grabar");
    return false;
  }

  {
    storage::Guard guard;
    LittleFS.remove(storage_recording::kRecordingTempPath);
    file_ = LittleFS.open(storage_recording::kRecordingTempPath, "w");
    if (!file_) {
      release_mutation();
      post(ui::Event::AudioStartFailed, "Audio: no se pudo crear el archivo temporal");
      return false;
    }
    std::uint8_t header[recording::kWavHeaderBytes];
    recording::make_wav_header(header, format.sample_rate, 0);
    if (file_.write(header, sizeof header) != sizeof header) {
      file_.close();
      LittleFS.remove(storage_recording::kRecordingTempPath);
      release_mutation();
      post(ui::Event::AudioStartFailed, "Audio: no se pudo escribir la cabecera WAV");
      return false;
    }
  }

  if (!init_i2s(format.sample_rate)) {
    {
      storage::Guard guard;
      file_.close();
      LittleFS.remove(storage_recording::kRecordingTempPath);
    }
    release_mutation();
    post(ui::Event::AudioStartFailed, "Audio: no se pudo iniciar I2S");
    return false;
  }

  pcm_bytes_ = 0;
  frame_.active = true;
  frame_.paused = false;
  frame_.available = false;
  frame_.elapsed_seconds = 0;
  frame_.max_seconds = format.max_seconds;
  frame_.sample_rate = format.sample_rate;
  running_ = true;
  last_publish_ms_ = millis();
  publish();
  post(ui::Event::RecordingStarted, "Cola de UI llena: se pierde RecordingStarted");
  return true;
}

void Recorder::finish() {
  stop_i2s();
  running_ = false;
  frame_.active = false;
  frame_.paused = false;
  if (pcm_bytes_ == 0) {
    {
      storage::Guard guard;
      file_.close();
      LittleFS.remove(storage_recording::kRecordingTempPath);
    }
    release_mutation();
    frame_.max_seconds = 0;
    frame_.sample_rate = 0;
    publish();
    post(ui::Event::AudioSaveFailed, "Audio: grabación vacía descartada");
    return;
  }

  bool saved = false;
  {
    storage::Guard guard;
    std::uint8_t header[recording::kWavHeaderBytes];
    recording::make_wav_header(header, frame_.sample_rate, pcm_bytes_);
    saved = file_.seek(0, SeekSet) &&
            file_.write(header, sizeof header) == sizeof header;
    file_.flush();
    file_.close();
    if (saved && !LittleFS.exists(storage_recording::kRecordingPath)) {
      saved = LittleFS.rename(storage_recording::kRecordingTempPath,
                              storage_recording::kRecordingPath);
    } else {
      saved = false;
    }
    if (!saved) LittleFS.remove(storage_recording::kRecordingTempPath);
  }

  release_mutation();
  frame_.max_seconds = 0;
  frame_.sample_rate = 0;
  frame_.available = saved;
  publish();
  post(saved ? ui::Event::AudioSaved : ui::Event::AudioSaveFailed,
       saved ? "Cola de UI llena: se pierde AudioSaved"
             : "Audio: no se pudo guardar o finalizar el WAV");
}

void Recorder::fail(ui::Event event, const char* log_line) {
  stop_i2s();
  running_ = false;
  frame_.active = false;
  frame_.paused = false;
  frame_.max_seconds = 0;
  frame_.sample_rate = 0;
  {
    storage::Guard guard;
    file_.close();
    LittleFS.remove(storage_recording::kRecordingTempPath);
  }
  release_mutation();
  if (log_ != nullptr) log_->log(log_line);
  publish();
  post(event, "Cola de UI llena: se pierde el error de grabación");
}

void Recorder::post(ui::Event event, const char* failure_line) {
  app::post(ui_out_, event, log_, failure_line);
}

void Recorder::publish() {
  view_->publish_recording(frame_);
}

void Recorder::release_mutation() {
  if (!mutation_held_) return;
  storage::end_mutation();
  mutation_held_ = false;
}

void Recorder::poll() {
  if (!running_) return;
  if (!frame_.paused) {
    std::int32_t samples[kReadSamples];
    std::size_t bytes_read = 0;
    const esp_err_t result =
        i2s_read(kI2sPort, samples, sizeof samples, &bytes_read, pdMS_TO_TICKS(20));
    if (result == ESP_OK && bytes_read > 0) {
      const std::size_t count = std::min<std::size_t>(bytes_read / sizeof(samples[0]),
                                                       kReadSamples);
      std::uint8_t pcm[kReadSamples * 2];
      for (std::size_t i = 0; i < count; ++i) {
        const std::uint16_t value =
            static_cast<std::uint16_t>(recording::sample_to_pcm16(samples[i]));
        pcm[2 * i] = static_cast<std::uint8_t>(value);
        pcm[2 * i + 1] = static_cast<std::uint8_t>(value >> 8);
      }
      const std::size_t pcm_size = count * 2;
      bool written = false;
      {
        storage::Guard guard;
        written = file_ && file_.write(pcm, pcm_size) == pcm_size;
      }
      if (!written) {
        fail(ui::Event::AudioSaveFailed, "Audio: fallo al escribir muestras en LittleFS");
        return;
      }
      pcm_bytes_ += static_cast<std::uint32_t>(pcm_size);
      frame_.elapsed_seconds = pcm_bytes_ / (frame_.sample_rate * 2);
      if (pcm_bytes_ >= frame_.max_seconds * frame_.sample_rate * 2) {
        finish();
        return;
      }
    } else if (result != ESP_OK && result != ESP_ERR_TIMEOUT) {
      fail(ui::Event::AudioSaveFailed, "Audio: error al leer muestras I2S");
      return;
    }
  }

  const std::uint32_t now = millis();
  if (static_cast<std::uint32_t>(now - last_publish_ms_) >= 1000) {
    last_publish_ms_ = now;
    publish();
  }
}

std::uint32_t Recorder::poll_interval_ms() const {
  return running_ && !frame_.paused ? 20 : 1000;
}

}  // namespace leeloo::audio_recorder
