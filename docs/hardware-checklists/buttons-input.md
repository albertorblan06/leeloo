# Temporary input: 3 push buttons (replaces the EC11 encoder)

Status: **proposed wiring, nothing wired or flashed yet.** Temporary substitute for the EC11 during the `dev` experiments; the encoder stays the target in `docs/ARCHITECTURE.md`.

## Mapping
| Button | Action | GPIO | Notes |
|---|---|---|---|
| UP | previous menu item (later: WPM +25) | GPIO4 | RTC-capable, deep-sleep wake possible |
| DOWN | next menu item (later: WPM -25) | GPIO5 | RTC-capable |
| SELECT | confirm; long press = back / sleep | GPIO6 | RTC-capable, wake source |

GPIO4-6 avoid: strapping pins (0, 3, 45, 46), octal PSRAM/flash (26-37 on N16R8), USB (19, 20), UART0 (43, 44), I2C (8, 9) and the onboard RGB LED (48).

## Wiring (all connections)
Each button is a 4-pin tactile switch (6x6 mm). Pins on the same side are shorted internally; pressing joins the two sides. Wire **diagonally** (pin 1 and pin 3) so it works whichever way the part is rotated.

| From | To |
|---|---|
| UP pin 1 | GPIO4 |
| UP pin 3 | GND |
| DOWN pin 1 | GPIO5 |
| DOWN pin 3 | GND |
| SELECT pin 1 | GPIO6 |
| SELECT pin 3 | GND |

### OLED 1.3" 128x64 (I2C, 4 pins, unchanged)
| OLED pin | ESP32-S3 DevKitC |
|---|---|
| VCC | 3V3 |
| GND | GND |
| SDA | GPIO8 |
| SCL | GPIO9 |

- The pin order printed on the module varies (often GND, VCC, SCL, SDA): read the silkscreen, do not assume.
- Most modules already have I2C pull-ups on board; add 4.7 kΩ to 3V3 on SDA and SCL only if the scan finds nothing.
- Address is 0x3C or 0x3D (the firmware scans both). Power from 3V3 even if the module accepts 5 V, so the I2C levels match the ESP32.

### Full pin summary
| GPIO | Use |
|---|---|
| 4 | UP button (to GND) |
| 5 | DOWN button (to GND) |
| 6 | SELECT button (to GND) |
| 8 | OLED SDA |
| 9 | OLED SCL |
| 3V3 | OLED VCC |
| GND | OLED GND + 3 button commons |

- No external resistors: use the ESP32 internal pull-up (`INPUT_PULLUP`). Idle = HIGH, pressed = LOW.
- Optional: 100 nF ceramic between each GPIO and GND for hardware debounce.
- Do not connect anything to 3V3 for the buttons.

## Firmware notes (when implemented)
- Keep the "no polling" rule: `attachInterrupt(pin, isr, CHANGE)` pushes events to a queue; debounce (~20 ms) and long-press (~800 ms, to tune) in the input task.
- Pure logic (debounce, long-press classification) goes in `lib/` with Unity tests; this checklist covers the hardware side.

## Menu test (firmware on `dev`)
- [ ] Flash `pio run -e esp32s3 -t upload`, monitor at 115200. Each press prints `ARRIBA|ABAJO|SELECT corto|largo` on serial.
- [ ] Main menu: UP and DOWN move the highlight; the highlight wraps from last to first and back.
- [ ] SELECT short on "Probar botones", "Contador" or "Acerca de" opens that screen.
- [ ] "Probar botones": each press updates the last event and its counter (A/B/S); SELECT long returns to the menu.
- [ ] "Contador": UP +1, DOWN -1, SELECT short +1; SELECT long returns.
- [ ] "Acerca de": any short SELECT returns to the menu.
- [ ] A press of ~1 s reports `largo`; a press under ~0.8 s reports `corto`. Adjust `kLongPressMs` if the feel is wrong.
- [ ] No double events from one press (debounce 20 ms). Note any bounce seen on serial: ______

## Checks
- [ ] Idle level of GPIO4/5/6 reads HIGH.
- [ ] Each press reads LOW, one event per press (no double events from bounce).
- [ ] Holding SELECT is classified as long press, releasing before the threshold as a short press.
- [ ] Two buttons pressed together do not reset or brown out the board.
- [ ] Result recorded here: date ______, board ______, outcome ______
