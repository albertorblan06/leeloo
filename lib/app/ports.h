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
