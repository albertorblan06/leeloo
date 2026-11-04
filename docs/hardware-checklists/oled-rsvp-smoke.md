# OLED RSVP smoke test

Firmware: `src/main.cpp` on branch `dev`. Shows a hardcoded text (a power-distribution paragraph, ~60 words) word by word at 200 WPM, in a loop.
Record the result of each run below (date, board, module, outcome).

## Setup
- [ ] OLED `VCC` to 3V3 (check the module's voltage range), `GND` to GND, `SDA` to GPIO8, `SCL` to GPIO9.
- [ ] `pio run -e esp32s3 -t upload`, then `pio device monitor` (115200 baud, UART port of the DevKitC; USB-CDC is off).

## Checks
- [ ] Serial prints `I2C device at 0x3C` (or `0x3D`). Record address: ______
- [ ] No `No OLED found` message.
- [ ] Controller: SH1106 (default in code). If the image is shifted ~2 px or the edges show garbage, switch to `U8G2_SSD1306_128X64_NONAME_F_HW_I2C`. Real controller: ______
- [ ] Text is sharp, not mirrored, not rotated.
- [ ] Accents and signs render correctly: `alimentación`, `Llevamos`, `raíles`, `¿Por`, `energía`, `eléctrico`, `estabilizando`.
- [ ] The two center marks (x = 64, above and below the word) are visible.
- [ ] Only the focal letter is hollow: a 1 px white contour following the glyph shape, black inside. It is centered on the marks and does not touch neighboring letters or accents.
- [ ] Long words are split with a trailing hyphen (e.g. `alimentación`, `distribución`, `microcontrolador`). The hollow letter is always the one under the center marks, and each piece is legible at 18 pt. If a piece clips or the split reads badly, note the word.
- [ ] Pace looks like ~200 WPM (one word every 300 ms); measure 10 loops with a stopwatch if in doubt. Measured: ______
- [ ] Pauses (at 200 WPM, measured or by ear): a word ending in `,` `;` `:` or a closing quote/paren/dash holds ~600 ms; one ending in `.` `?` `!` `…` holds ~900 ms; a blank line after `fallo.` holds ~1,5 s. Opening marks (`¿`, `¡`, `(`, `—`) do not pause. Words longer than 8 letters hold slightly longer (~375 ms). Note any pause that feels wrong.
- [ ] After the last word the screen blanks for ~1 s and the phrase restarts.
- [ ] No flicker or frozen frames over 2 minutes.

## Run log
| Date | Board | OLED module | I2C addr | Controller | Result |
|------|-------|-------------|----------|------------|--------|
|      |       |             |          |            |        |
