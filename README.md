# Leeloo

Leeloo is a pocket-sized RSVP (Rapid Serial Visual Presentation) reader for Spanish text, built on an ESP32-S3 with a small monochrome OLED and three buttons. Books are converted from PDF on the phone and sent to the device over its own Wi-Fi; the device also records voice notes, which the phone can transcribe and summarize.

## Status

The firmware, the phone page and the unit tests are implemented and have been flashed and exercised on an ESP32-S3 N16R8 development board: boot animation and menus, reading, adding a book from the phone, recording, sending the recording to the phone, and transcription with Groq. The hardware checklists in [`docs/hardware-checklists/`](docs/hardware-checklists/) are only partly ticked, the enclosure is still being designed, and the battery figures are estimates (see below).

## What it does

- **Reads** books one word at a time at 100-800 words per minute (PPM), with the focal letter marked, pauses for punctuation and paragraphs, long words split at syllable boundaries, sentence jumps while paused, and the position remembered per book. UP/DOWN change the speed while reading; Ajustes sets how much each press changes it.
- **Adds books** from the phone: join the `Leeloo` network, enter the PIN shown on the screen, pick a PDF; the page converts it in the browser and sends plain text to the device.
- **Records** voice notes (16 kHz mono WAV, as long as the free space allows), sends them to the phone over the same Wi-Fi, and the phone page can **transcribe and summarize** them with the Groq API (your own key, kept only in the phone's browser; needs mobile data).
- **Switches off** with a long press on the main menu (deep sleep) and wakes by holding SELECT.
- **Menus**: Leer, Biblioteca (añadir / borrar libro), Grabadora, Ajustes (brillo, salto por pulsación, Wi-Fi), Info. A long press always goes back one level.

## Hardware

- ESP32-S3-DevKitC-1 class board, N16R8 (16 MB flash, 8 MB PSRAM).
- 1.3" 128x64 SH1106 I2C OLED (SDA GPIO8, SCL GPIO10).
- Three buttons to GND: UP GPIO5, DOWN GPIO4, SELECT GPIO6 (also the wake source).
- INMP441 I2S microphone: BCLK GPIO15, WS GPIO16, data GPIO17.
- 1S LiPo -> TP4056 -> SX1308 boost to the 5V pin. Rough estimate: about 14 hours of reading-heavy use per charge, about 2 days with a mixed day when "off" is the development board's deep sleep, which still draws about 20 mA; see [`docs/hardware-checklists/power.md`](docs/hardware-checklists/power.md).

## Build and test

```sh
pio test -e native           # unit tests of the pure logic in lib/ (host, no board)
pio run -e esp32s3           # firmware build (compile only)
cd web && npm test           # phone page tests (Vitest)
cd web && npm run build      # gzip the page into web/dist (the LittleFS image content)

pio run -e esp32s3 -t upload                              # flash the firmware
pio run -e esp32s3 -t uploadfs --upload-port <port>       # write the web page; erases the books on the device
```

Pure logic lives in `lib/<module>/` and never includes Arduino or FreeRTOS headers; hardware goes through small interfaces so the native tests can fake it. `src/` holds the board-bound adapters and the task wiring. The test policy and coverage targets are in [`CLAUDE.md`](CLAUDE.md).

## Documents

- [`docs/DEVICE-MENU.md`](docs/DEVICE-MENU.md): the on-device menu and navigation rules.
- [`docs/DESIGN.md`](docs/DESIGN.md): OLED design rules, the boot logo (adapted from Loudop).
- [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) and [`docs/ARCHITECTURE-ANSWERS.md`](docs/ARCHITECTURE-ANSWERS.md): technical baseline and the recorded product decisions.
- [`docs/WEB-API.md`](docs/WEB-API.md): the contract between the phone page and the firmware.
- [`docs/ROADMAP.md`](docs/ROADMAP.md) and [`docs/IMPLEMENTATION-PLAN.md`](docs/IMPLEMENTATION-PLAN.md): phases and milestones.
- [`docs/hardware-checklists/`](docs/hardware-checklists/): what to verify on the real board, per feature.

## Known limits

- Transcription needs the phone to reach Groq over mobile data while joined to the `Leeloo` network, which has no internet; Android and iOS differ in whether they do.
- The device deletes its copy of a recording once the phone has received it all, and the page keeps it only in memory until it is downloaded.
- "Off" is deep sleep on a development board, not a power switch; a hardware switch would remove the remaining drain.
- The battery figures are estimates, not measurements.
