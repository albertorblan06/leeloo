#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "display_ui.h"
#include "power.h"
#include "settings_store.h"
#include "storage_books.h"
#include "tasks.h"

// Boot only. Every service runs in its own task (src/tasks.cpp); the logic is in lib/app.
// Covered by docs/hardware-checklists/menu.md, reading.md and rtos.md.

void setup() {
  // First: after waking from "off" nothing else may start unless SELECT is still held.
  leeloo::power::confirm_wake_or_sleep();
  Serial.begin(115200);
  const leeloo::settings::Settings boot = leeloo::settings_store::load();

  if (!leeloo::display::init()) Serial.println("No OLED at 0x3C/0x3D: input still reports on serial.");
  // Before the tasks exist, so no other task can be using the bus yet.
  leeloo::display::set_contrast(leeloo::settings::display_register(boot.brightness_percent));
  leeloo::display::boot_animation_step();  // first frame at once, so the panel is never blank

  // Mounting runs behind the animation, which is clocked by time: if the mount is slow the logo
  // skips frames instead of making the boot longer.
  if (!leeloo::storage::mount()) Serial.println("LittleFS no montó: libros no disponibles.");
  while (!leeloo::display::boot_animation_step()) delay(1);

  leeloo::tasks::start(boot);
}

// The Arduino loop task has nothing left to do; delete it so it does not wake for nothing.
void loop() { vTaskDelete(nullptr); }
