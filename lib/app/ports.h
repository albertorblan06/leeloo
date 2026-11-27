#pragma once

#include <cstddef>
#include <cstdint>

#include "messages.h"
#include "rtos.h"
#include "settings.h"

// What the services need from the hardware-bound modules in src/. The device
// adapters wrap storage_books, upload_server, settings_store and display_ui;
// the tests use fakes.
namespace leeloo::app {

class ILog {
 public:
  virtual ~ILog() = default;
  virtual void log(const char* line) = 0;
};

class IPins {
 public:
  virtual ~IPins() = default;
  // Current level of a button pin; true when released.
  virtual bool read_high(std::uint8_t button) = 0;
};

struct BookBuffer {
  const char* data = nullptr;
  std::size_t size = 0;
};

class ILibrary {
 public:
  virtual ~ILibrary() = default;
  virtual std::size_t book_count() = 0;
  virtual bool remove_at(std::size_t index) = 0;
  // Loads book `index` and copies its title, under one library lock so the index
  // names one book. The buffer stays valid until close(). At most one book is open.
  virtual bool open(std::size_t index, char* title, std::size_t title_cap, BookBuffer* out) = 0;
  virtual void close() = 0;
  virtual bool load_position(const char* title, std::size_t* offset) = 0;
  virtual bool save_position(const char* title, std::size_t offset) = 0;
};

class ISettingsStore {
 public:
  virtual ~ISettingsStore() = default;
  virtual bool save(const settings::Settings& settings) = 0;
};

// The last-used reading speed, one value shared by all books. Kept apart from
// Settings because RsvpService owns it: it is the only one that knows the speed.
class ISpeedStore {
 public:
  virtual ~ISpeedStore() = default;
  virtual std::uint16_t load() = 0;
  virtual bool save(std::uint16_t wpm) = 0;
};

// The microphone, as 16-bit mono PCM at the requested rate. The adapter owns the I2S driver.
class IMic {
 public:
  virtual ~IMic() = default;
  virtual bool start(std::uint32_t sample_rate) = 0;
  virtual void stop() = 0;
  virtual bool pause() = 0;
  virtual bool resume() = 0;
  // Reads up to `max` samples, waiting a few ms at most. Returns the count (0 when none
  // arrived yet) or -1 on a driver error.
  virtual int read(std::int16_t* out, std::size_t max) = 0;
};

// Where a recording lives: one temporary file while capturing, one finished WAV.
class IRecordingStore {
 public:
  virtual ~IRecordingStore() = default;
  virtual bool ready() = 0;           // storage mounted
  virtual bool exists() = 0;          // a finished WAV is stored
  virtual std::size_t free_bytes() = 0;
  // Capture owns the storage between lock() and unlock(), so book uploads and deletes
  // are refused meanwhile. False when another storage operation is running.
  virtual bool lock() = 0;
  virtual void unlock() = 0;
  virtual bool open_temp(std::uint32_t sample_rate) = 0;  // creates the file with a header
  virtual bool write(const std::uint8_t* data, std::size_t size) = 0;
  // Patches the header for `pcm_bytes`, closes and renames to the final WAV.
  virtual bool finish(std::uint32_t sample_rate, std::uint32_t pcm_bytes) = 0;
  virtual void abort() = 0;           // closes and removes the temporary file
  virtual bool remove() = 0;          // deletes the finished WAV; true when none is left
  virtual void cleanup_temp() = 0;    // leftovers of an interrupted capture
};

class IDisplayControl {
 public:
  virtual ~IDisplayControl() = default;
  virtual void set_contrast(std::uint8_t register_value) = 0;
};

class INet {
 public:
  virtual ~INet() = default;
  virtual bool running() = 0;
  virtual bool start() = 0;  // false when the AP or the HTTP server cannot start
  virtual void stop() = 0;
  virtual void mark_activity() = 0;
  virtual std::uint32_t last_activity_ms() = 0;
};

// Sends a message and logs it when the queue stays full, so a drop is never silent.
template <typename T>
void post(rtos::Queue<T>* queue, const T& item, ILog* log, const char* what) {
  if (!queue->send(item, kPostTimeoutMs) && log != nullptr) log->log(what);
}

}  // namespace leeloo::app
