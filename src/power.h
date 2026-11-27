#pragma once

// Switching off is deep sleep: everything stops and only SELECT (GPIO6) can wake the device.
// Hardware-bound; covered by docs/hardware-checklists/power.md.
namespace leeloo::power {

// Called first thing in setup(). After a wake-up by SELECT the button must still be held for
// about a second; a shorter press sends the device straight back to sleep, so a knock in a bag
// does not switch it on. Returns at once on any other boot (power-on, reset, flashing).
void confirm_wake_or_sleep();

// Stops Wi-Fi, switches the panel off, unmounts the filesystem and sleeps. Does not return.
// Call from the display task, which owns the I2C bus, once "Apagando..." has been drawn.
[[noreturn]] void enter_deep_sleep();

}  // namespace leeloo::power
