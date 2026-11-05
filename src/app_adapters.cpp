#include "app_adapters.h"

#include <Arduino.h>

#include <cstdio>
#include <cstring>

#include "display_ui.h"
#include "pins.h"
#include "settings_store.h"
#include "upload.h"
#include "upload_server.h"

namespace leeloo::adapters {

void SerialLog::log(const char* line) { Serial.println(line); }

bool GpioPins::read_high(std::uint8_t button) {
  if (button >= pins::kButtonCount) return true;  // an unknown button reads as released
  return digitalRead(pins::kButtonPins[button]) == HIGH;
}

std::size_t StorageLibrary::book_count() { return storage::book_count(); }

bool StorageLibrary::remove_at(std::size_t index) { return storage::remove_at(index); }

// The library lock spans the lookup and the load, so the index names one book.
bool StorageLibrary::open(std::size_t index, char* title, std::size_t title_cap, app::BookBuffer* out) {
  close();
  storage::Guard guard;
  if (!storage::title_at(index, title, title_cap)) return false;
  if (!storage::load_text(index, &text_)) return false;
  out->data = text_.data;
  out->size = text_.size;
  return true;
}

void StorageLibrary::close() { storage::free_text(&text_); }

bool StorageLibrary::load_position(const char* title, std::size_t* offset) {
  return storage::load_position(title, offset);
}

bool StorageLibrary::save_position(const char* title, std::size_t offset) {
  return storage::save_position(title, offset);
}

bool NvsSettings::save(const settings::Settings& settings) { return settings_store::save(settings); }

void DisplayContrast::set_contrast(std::uint8_t register_value) { view_->request_contrast(register_value); }

bool SoftAp::running() { return upload_server::running(); }

bool SoftAp::start() {
  char pin[upload::kPinDigits + 1];
  if (!upload_server::start(pin)) return false;
  std::snprintf(pin_, sizeof pin_, "%s", pin);
  return true;
}

void SoftAp::stop() {
  upload_server::stop();
  std::memset(pin_, 0, sizeof pin_);
}

void SoftAp::mark_activity() { upload_server::mark_activity(); }

std::uint32_t SoftAp::last_activity_ms() { return upload_server::last_activity_ms(); }

app::NetStatus SoftAp::status() const {
  app::NetStatus status;
  status.running = upload_server::running();
  std::snprintf(status.pin, sizeof status.pin, "%s", status.running ? pin_ : "");
  return status;
}

}  // namespace leeloo::adapters
