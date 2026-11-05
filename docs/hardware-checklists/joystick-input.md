# Temporary input: analog joystick (replaces the 3 buttons)

Firmware: `src/main.cpp` on branch `dev`. Supersedes `buttons-input.md` for the menu test.

## Wiring
| Joystick pin | ESP32-S3 | Use |
|---|---|---|
| GND | GND | ground |
| +5V / VCC | **3V3** | power (3.3 V so the axes never exceed the ADC range) |
| VRy | GPIO4 (ADC1) | vertical: UP / DOWN |
| VRx | GPIO5 (ADC1) | horizontal: LEFT / RIGHT |
| SW | GPIO7 | push: SELECT (to GND, internal pull-up) |

OLED unchanged: VCC 3V3, GND, SDA GPIO8, SCL GPIO9. ADC1 is used because ADC2 stops working while Wi-Fi is on.

## Behavior
- Center is measured at boot (average of 16 samples): **do not touch the stick while the board resets.**
- A push beyond 800 counts from the center registers once; it re-arms inside 500 counts. No auto-repeat. Axis sampling every 10 ms.
- Y low = UP, Y high = DOWN. If it feels inverted, swap `kUp`/`kDown` in `axis_task`.
- SW: short press < 800 ms, long press >= 800 ms (same as the buttons).

## Checks
- [ ] OLED shows the menu after boot; serial (UART port) prints `Joystick centers: Y=… X=…` close to 2048. Record: Y ____ X ____
- [ ] Stick pushed up/down moves the highlight, one step per push, wrapping at the ends.
- [ ] Direction is not inverted (up moves up). Otherwise swap in code.
- [ ] Menu does not move by itself at rest (no drift).
- [ ] SW short opens a screen; SW long goes back from a sub-screen.
- [ ] "Probar joystick" counts A, B, I, D and S correctly; diagonals do not register twice on the same axis.
- [ ] Note the range seen when pushed fully: Y ____ to ____, X ____ to ____ (serial or OLED), to tune the 800/500 thresholds.
- [ ] Press SW 30 times quickly in "Probar joystick": the S counter reads 30 (no lost presses). Record: S = ____
- [ ] Result recorded here: date ______, board ______, outcome ______
