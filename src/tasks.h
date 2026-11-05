#pragma once

#include "settings.h"

// The firmware's task set (docs/ARCHITECTURE.md §4.1). Hardware-bound: covered by
// the build and docs/hardware-checklists/rtos.md. The logic each task runs lives in lib/app.
namespace leeloo::tasks {

// Creates the queues, the services, the tasks and the button interrupts. Call once
// from setup(), after storage, settings and the display are ready. If the RTOS
// cannot allocate a queue or a task it logs the reason and halts: nothing works without them.
void start(const settings::Settings& boot);

}  // namespace leeloo::tasks
