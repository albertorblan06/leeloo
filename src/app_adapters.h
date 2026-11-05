#pragma once

#include <cstddef>
#include <cstdint>

#include "ports.h"
#include "storage_books.h"
#include "view_model.h"

// Device implementations of the ports in lib/app/ports.h. Each wraps one hardware-bound
// module and adds nothing else. Covered by docs/hardware-checklists/menu.md and rtos.md.
namespace leeloo::adapters {

class SerialLog : public app::ILog {
 public:
  void log(const char* line) override;
};

// Button pins: UP, DOWN, SELECT, all active low with the internal pull-up.
class GpioPins : public app::IPins {
 public:
  bool read_high(std::uint8_t button) override;
};

class StorageLibrary : public app::ILibrary {
 public:
  std::size_t book_count() override;
  bool remove_at(std::size_t index) override;
  bool open(std::size_t index, char* title, std::size_t title_cap, app::BookBuffer* out) override;
  void close() override;
  bool load_position(const char* title, std::size_t* offset) override;
  bool save_position(const char* title, std::size_t offset) override;

 private:
  storage::BookText text_;  // the one open book, in PSRAM
};

class NvsSettings : public app::ISettingsStore {
 public:
  bool save(const settings::Settings& settings) override;
};

// Contrast is a bus write, and the display task owns the bus: the request goes
// through the view model and is applied there.
class DisplayContrast : public app::IDisplayControl {
 public:
  explicit DisplayContrast(app::ViewModel* view) : view_(view) {}
  void set_contrast(std::uint8_t register_value) override;

 private:
  app::ViewModel* view_;
};

class SoftAp : public app::INet {
 public:
  bool running() override;
  bool start() override;
  void stop() override;
  void mark_activity() override;
  std::uint32_t last_activity_ms() override;

  // Current AP state for the display. Call from the net task, the only one that starts or stops the AP.
  app::NetStatus status() const;

 private:
  char pin_[16] = {};
};

}  // namespace leeloo::adapters
