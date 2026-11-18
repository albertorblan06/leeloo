#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "display_ui.h"
#include "settings_store.h"
#include "storage_books.h"
#include "tasks.h"

// Boot only. Every service runs in its own task (src/tasks.cpp); the logic is in lib/app.
// Covered by docs/hardware-checklists/menu.md, reading.md and rtos.md.

void setup() {
  Serial.begin(115200);
  const leeloo::settings::Settings boot = leeloo::settings_store::load();

  if (!leeloo::display::init()) {
    Serial.println("No OLED at 0x3C/0x3D: input still reports on serial.");
  } else {
    leeloo::display::show_splash();
  }
  // Before the tasks exist, so no other task can be using the bus yet.
  leeloo::display::set_contrast(leeloo::settings::display_register(boot.brightness_percent, boot.contrast_percent));
  if (!leeloo::storage::mount()) Serial.println("LittleFS no montó: libros no disponibles.");
  if (leeloo::display::ready()) delay(900);

  leeloo::tasks::start(boot);
}

// The Arduino loop task has nothing left to do; delete it so it does not wake for nothing.
void loop() { vTaskDelete(nullptr); }
