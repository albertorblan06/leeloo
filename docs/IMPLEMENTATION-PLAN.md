# Leeloo Implementation Plan

This plan follows the locked baseline in [`ARCHITECTURE.md`](ARCHITECTURE.md). Hardware implementation must validate the selected parts before firmware behavior is treated as final.

## Milestone 1: Project and toolchain foundation

- Create the Arduino-ESP32 project structure.
- Confirm the exact ESP32-S3 N16R8 module, flash map, PSRAM visibility, and USB serial flashing.
- Add formatting, static checks, host-side unit-test execution, and a reproducible build command.
- Add configuration constants for board pins, display, storage limits, and feature flags.

**Exit evidence:** clean build, flash, serial boot log, and a passing test command on the development Mac.

## Milestone 2: Hardware smoke test

- Wire the OLED over I2C and confirm controller identity/configuration.
- Wire the EC11 phases and push-button to interrupt-capable GPIOs.
- Validate encoder interrupts under fast rotation and switch bounce.
- Validate LiPo/TP4056 power, charging, protection, and battery measurement.

**Exit evidence:** display test pattern, measured input events without polling, safe charge/power behavior, and documented pinout.

## Milestone 3: Firmware foundation

- Implement boot, logging, filesystem mount, settings load, watchdog, and error states.
- Implement the FreeRTOS input task and semantic event queue.
- Implement the UI state machine and Spanish text resources.
- Implement display rendering and brightness settings.

**Exit evidence:** boot-to-menu flow, deterministic input behavior, and unit tests for event classification/state transitions.

## Milestone 4: Storage and book service

- Implement the single-file metadata parser and UTF-8 validation.
- Implement library enumeration, selection, deletion, free-space checks, and atomic upload replacement.
- Implement settings and per-book byte-offset persistence.
- Add boot recovery for incomplete temporary files and mount failures.

**Exit evidence:** host tests for malformed files, metadata, limits, atomic replacement, offset recovery, and storage-full behavior.

## Milestone 5: RSVP engine

- Implement strict word tokenization over normalized UTF-8 text.
- Implement WPM timing, punctuation pauses, focal-letter selection, and speed adjustment.
- Implement pause/resume, word seeking, completion percentage, estimated page, and end-of-book actions.
- Validate byte offsets never split UTF-8 code points.

**Exit evidence:** deterministic host-side timing/token tests and device reading of a known Spanish fixture book.

## Milestone 6: Browser upload application

- Build the mobile-first upload page served from the device.
- Implement browser-side PDF extraction, normalization, metadata entry, UTF-8/LF serialization, and progress/cancel/retry states.
- Implement per-session PIN authentication and upload protocol.
- Handle unsupported/scanned PDFs with a Spanish-facing error and no partial book commit.

**Exit evidence:** successful iOS Safari/Android Chrome workflows using representative selectable-text PDFs and explicit failure tests.

## Milestone 7: Integration and power behavior

- Integrate menu, Library, Upload Book, Settings, Reading, Paused, and Error states.
- Add temporary AP lifecycle and long-press cancellation.
- Add battery estimate, low-battery behavior, deep sleep, and encoder wake.
- Verify progress persistence on normal exit and controlled power interruption.

**Exit evidence:** end-to-end acceptance checklist passes on the real board with a representative book.

## Milestone 8: Hardening and release candidate

- Run unit tests for all implemented pure logic and repeat hardware regression checks.
- Measure memory, display refresh, upload throughput, storage headroom, and battery behavior.
- Review dependency licenses and select the project license.
- Update README and architecture notes with measured constraints and known limitations.

**Exit evidence:** reproducible build/test instructions, known-issues list, and a tagged prototype release candidate.

## Cross-cutting validation rules

- No encoder pin polling may be introduced.
- No PDF bytes may be sent to or parsed by the ESP32.
- All upload commits must be atomic and validated before replacing a book.
- All persistent offsets must be UTF-8-safe.
- Every new pure logic module requires unit tests in the same change.
- User-facing strings belong in Spanish; repository/code documentation remains in English.
