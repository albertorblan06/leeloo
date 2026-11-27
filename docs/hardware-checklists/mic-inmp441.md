# INMP441 microphone (bring-up)

The production firmware records with this wiring (`audio-recording.md`). This file is the single source for it.

Test firmware: `pio run -e mic-test -t upload` (source in `tools/mic_test/`). Serial over the native USB port at 115200.

## Wiring

| INMP441 | ESP32-S3 |
|---|---|
| VDD | 3V3 |
| GND | GND |
| SCK | GPIO15 |
| WS | GPIO16 |
| SD | GPIO17 |
| L/R | GND (left channel) |

## Checklist

- [x] Test firmware prints `peak <n>` lines every 100 ms, values fluctuate (seen 30 k to 400 k, 2026-11-24).
- [ ] Silence level measured in a quiet room (note the typical peak here: ____).
- [ ] Clap or speech close to the mic raises the peak clearly above the silence level.
- [ ] With SD disconnected the peak does not follow sound (confirms it is not noise pickup).

Restore the app afterwards with `pio run -e esp32s3 -t upload`, or write a full-flash backup with `esptool.py write_flash 0 <file>`.
