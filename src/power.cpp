#include "power.h"

#include <Arduino.h>
#include <LittleFS.h>
#include <driver/rtc_io.h>
#include <esp_sleep.h>
#include <esp_system.h>
#include <esp_task_wdt.h>

#include "display_ui.h"
#include "pins.h"
#include "storage_books.h"
#include "upload_server.h"

namespace leeloo::power {
namespace {

constexpr gpio_num_t kWakePin = static_cast<gpio_num_t>(pins::kSelectPin);
// Time SELECT must stay down after the wake-up. The boot itself takes about 0.4 s of it, so the
// whole press is about a second.
constexpr std::uint32_t kWakeHoldMs = 700;
// SELECT must be up before sleeping, or the wake level is already present and the device
// would wake at once. The user has just released it to confirm, so this is normally instant.
constexpr std::uint32_t kReleaseWaitMs = 2000;

[[noreturn]] void sleep_now() {
  // The internal pull-up of pinMode(INPUT_PULLUP) is lost in deep sleep; the RTC one stays.
  // Sleeping without a wake source would leave the device dead until RESET, so if arming it
  // fails the safer outcome is a reboot into the normal menu.
  if (esp_sleep_enable_ext0_wakeup(kWakePin, 0) != ESP_OK) esp_restart();  // wake when SELECT goes low
  rtc_gpio_pullup_en(kWakePin);
  rtc_gpio_pulldown_dis(kWakePin);
  esp_deep_sleep_start();
  for (;;) {}  // esp_deep_sleep_start() does not return; keeps the compiler honest about [[noreturn]]
}

}  // namespace

void confirm_wake_or_sleep() {
  if (esp_sleep_get_wakeup_cause() != ESP_SLEEP_WAKEUP_EXT0) return;
  pinMode(pins::kSelectPin, INPUT_PULLUP);
  const std::uint32_t started = millis();
  while (millis() - started < kWakeHoldMs) {
    if (digitalRead(pins::kSelectPin) == HIGH) sleep_now();  // released early: a knock, not a wish
    delay(5);
  }
}

void enter_deep_sleep() {
  // The display task calls this and never feeds the watchdog again. Stopping the server or
  // waiting for the library lock can take a few seconds; without this a slow stop would end
  // in a watchdog reset (a reboot to the splash) instead of sleep.
  esp_task_wdt_delete(nullptr);
  upload_server::stop();  // httpd and the access point; harmless when they are not running
  display::power_save();
  // Held until the device sleeps: no file operation can be half-done, and no other task can
  // reach the filesystem after it is closed. Other tasks that ask for it just wait.
  storage::Guard guard;
  if (storage::mounted()) LittleFS.end();
  pinMode(pins::kSelectPin, INPUT_PULLUP);
  const std::uint32_t started = millis();
  while (digitalRead(pins::kSelectPin) == LOW && millis() - started < kReleaseWaitMs) delay(5);
  // If SELECT is still down after kReleaseWaitMs the device wakes at once and, since the button
  // is still held, boots again. That is bounded and harmless; waiting longer would only trap
  // a user who rests a finger on the button.
  sleep_now();
}

}  // namespace leeloo::power
