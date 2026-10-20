# Leeloo

Embedded Spanish RSVP reader (ESP32-S3 firmware + phone web app). Architecture: `docs/ARCHITECTURE.md`. Delivery: `docs/ROADMAP.md`, `docs/IMPLEMENTATION-PLAN.md`.

## Unit-testing policy

- Every new feature and every bug fix ships with unit tests **in the same change**. For bugs, write a failing regression test first, then fix.
- Pure logic requires unit tests. Hardware-bound code (OLED, encoder ISR, power, Wi-Fi) requires an updated manual checklist in `docs/hardware-checklists/` instead.
- Never delete, skip or weaken a test to make it pass.
- Test names are English and behavior-oriented, e.g. `rejects_offset_inside_utf8_codepoint`.

### Frameworks and commands

- Firmware pure logic: PlatformIO `native` env with Unity — `pio test -e native`. Build check: `pio run -e esp32s3` (compile only; the board is provisional).
- Web app logic (from Phase 6, once the dependency is approved): Vitest — `npm test` (not set up yet).
- Both must pass before work is declared done.

### Structure

Pure logic lives in `lib/<module>/` and must not include `Arduino.h` or FreeRTOS headers. Reach hardware through thin interfaces (`IClock`, `IStorage`, `IDisplay`) so the native env can fake them.

### Coverage

- At least 90 % line coverage on `lib/` pure-logic modules; at least 80 % on web app logic.
- Changed lines must be covered. CI fails below the threshold.
- Hardware adapters and `src/main` are excluded.

### Required edge cases

Empty and oversize input; multibyte Spanish characters (á, ñ, ¿, ¡); offsets on UTF-8 code-point boundaries; WPM bounds 150–800 in steps of 25; atomic-write failure paths.

## Graph

After modifying code, run `graphify update .`.
