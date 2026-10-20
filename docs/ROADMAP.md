# Leeloo Roadmap

Phased delivery plan built on the locked baseline in [`ARCHITECTURE.md`](ARCHITECTURE.md). Milestones and their exit evidence live in [`IMPLEMENTATION-PLAN.md`](IMPLEMENTATION-PLAN.md); this document orders them into phases with measurable goals and the unit-testing policy (see [`../CLAUDE.md`](../CLAUDE.md)).

## Current state

Documentation only: architecture baseline, implementation plan and agent files. No source code, build configuration, tests or CI exist yet.

## Test strategy

| Target | Framework | Notes |
| --- | --- | --- |
| Firmware pure logic | PlatformIO `native` env + Unity | Bundled with `pio test`; no new dependency. Coverage with `gcov`/`gcovr`. |
| Web app logic | Vitest | Added only in Phase 6 and only after the dependency is approved. |
| On-device behavior (OLED, encoder ISR, power, Wi-Fi) | Manual checklists in `docs/hardware-checklists/` | Not unit-testable; recorded as exit evidence and excluded from coverage. |

Pure logic lives in `lib/<module>/` without `Arduino.h` or FreeRTOS includes. Hardware is reached through thin interfaces (for example `IClock`, `IStorage`, `IDisplay`) so the native environment can fake them.

## Phase 0 — Repo and test foundation (M1)

**Objective:** a reproducible build and a working, automated test command before any feature code.

**Tasks (in order):**
1. Add project `CLAUDE.md` with the testing policy.
2. Scaffold `platformio.ini` with `esp32s3` and `native` environments, `lib/`, `test/`, `.clang-format`.
3. Extend `.gitignore` (`.pio/`, `node_modules/`, coverage output).
4. Add one sample pure-logic module with one passing test to prove the toolchain.
5. Add a GitHub Actions workflow: `pio test -e native` and ESP32 build (coverage and format checks are added in Phase 5; `gcovr` and `clang-format` are not installed yet).
6. Confirm the N16R8 module, flash map, PSRAM visibility and USB flashing on the board.

**Status:** tasks 1–5 done (PlatformIO confirmed; `esp32s3` env is provisional, `platformio/espressif32@7.1.3`, board `esp32-s3-devkitc-1` from an Amazon listing, flash/PSRAM/partitions deliberately unset). Task 6 waits for the hardware. The first CI run on GitHub has not happened yet.

**Risks / decisions:** the `esp32-s3-devkitc-1` defaults gave a ~3.3 MB app partition in the first build, so they do not describe an N16R8 module; Phase 1 must verify the memory map and set flash, PSRAM and partitions. Arduino-ESP32 core version follows the pinned platform until Phase 1 confirms it.

**Done when:** clean build, `pio test -e native` passes locally and in CI on a pull request, and a serial boot log is captured from the board.

## Phase 1 — Hardware validation gates (M2)

**Objective:** verify every purchased part before firmware depends on it.

**Tasks (in order):**
1. Identify the OLED module and controller (SH1106 is only likely), voltage and pinout.
2. Wire the OLED over I2C and show a test pattern.
3. Wire the EC11 to interrupt-capable GPIOs; measure fast rotation and switch bounce.
4. Validate the LiPo/TP4056 charge, protection and battery measurement path **before** connecting the battery to the board.
5. Write the pinout document and hardware checklists.

**Risks / decisions:** wrong display controller (keep the display adapter isolated); battery safety.

**Done when:** test pattern on the verified OLED, interrupt-driven input events measured without polling, documented pinout, and a completed power checklist.

## Phase 2 — Firmware core, test-first (M3)

**Objective:** boot-to-menu with deterministic input handling.

**Tasks (in order):**
1. Configuration constants (pins, limits, feature flags).
2. Input event classifier (debounce, long-press) and FreeRTOS input task.
3. UI state machine.
4. Spanish string table.
5. Display adapter, font and brightness.
6. Boot, logging, watchdog and error states.

**Risks / decisions:** font coverage for Spanish glyphs; ISR/queue design.

**Done when:** boot-to-menu works on the device, state-transition and event-classification tests pass, and coverage on those modules is at least 90 %.

## Phase 3 — Storage and book service (M4)

**Objective:** books and progress are stored safely.

**Tasks (in order):**
1. UTF-8 validator.
2. Metadata parser (`TITLE`, `AUTHOR`, `PAGES`, `###START###`).
3. Limits as constants (1 MiB book, 20 books, 48-byte names).
4. Atomic temp-file-then-rename write and boot recovery.
5. Settings and per-book byte-offset persistence.
6. Library enumeration, deletion and free-space checks.

**Risks / decisions:** LittleFS partition capacity may change the starting limits.

**Done when:** host tests for malformed files, oversize input, storage-full, atomic replacement and offset recovery pass.

## Phase 4 — RSVP engine (M5)

**Objective:** correct, deterministic reading behavior.

**Tasks (in order):**
1. Word tokenizer over normalized UTF-8.
2. WPM timing (150–800, default 300, step 25) and punctuation pauses.
3. Focal-letter selection.
4. Pause/resume and word seeking.
5. Completion percentage and estimated page.
6. End-of-book actions.

**Risks / decisions:** pause weights need tuning with real reading.

**Done when:** timing and token tests are deterministic, a test proves offsets never split a UTF-8 code point over a Spanish fixture, and the fixture book reads correctly on the device.

## Phase 5 — Retrofit tests and CI hardening (parallel to Phases 2–4, closes before Phase 6)

**Objective:** nothing merged without tests, enforced automatically.

**Tasks (in order):**
1. Backfill tests for any logic merged without them.
2. Enforce coverage thresholds in CI.
3. Add a manual hardware-checklist template.
4. Add a format/lint check to CI.

**Risks / decisions:** coverage thresholds may need adjusting once real modules exist.

**Done when:** CI fails on a failing test or on coverage below threshold, and a policy audit (every module under `lib/` has tests) passes.

## Phase 6 — Browser upload application (M6)

**Objective:** phone-to-device book transfer.

**Tasks (in order):**
1. Decide the web stack and approve dependencies (pdf.js, Vitest).
2. PDF extraction and normalization.
3. Header serializer and UTF-8/LF output.
4. Per-session PIN authentication and upload protocol.
5. Progress, cancel, retry and error states (scanned PDF gives a Spanish error and no partial commit).

**Risks / decisions:** iOS Safari memory on large PDFs; pdf.js size and license against the 1 MiB book limit.

**Done when:** Vitest coverage on logic is at least 80 %, explicit failure tests pass, and uploads succeed from iOS Safari and Android Chrome.

## Phase 7 — Integration and power (M7)

**Objective:** the complete product flow on real hardware.

**Tasks (in order):**
1. Integrate Library, Upload Book, Settings, Reading, Paused and Error states.
2. Temporary AP lifecycle and long-press cancellation.
3. Battery estimate and low-battery behavior.
4. Deep sleep and encoder wake.
5. Progress persistence on normal exit and controlled power interruption.

**Risks / decisions:** wake-capable GPIO choice; core 0 / core 1 shared-state races.

**Done when:** the end-to-end acceptance checklist passes on the real board with a representative book.

## Phase 8 — Hardening and release candidate (M8)

**Objective:** a measured, documented prototype.

**Tasks (in order):**
1. Measure memory, OLED refresh, upload throughput, storage headroom and battery life.
2. Repeat hardware regression checks.
3. Review dependency licenses and select the project license.
4. Update README and architecture notes with measured limits and known issues.
5. Tag the release candidate.

**Risks / decisions:** project license choice.

**Done when:** reproducible build and test instructions exist, a known-issues list is published, and a prototype release candidate is tagged.

## Decisions needed

| Decision | Recommendation | Needed by |
| --- | --- | --- |
| PlatformIO vs arduino-cli | Decided: PlatformIO (already installed, bundles Unity) | Phase 0 |
| Web stack, pdf.js and Vitest dependencies | Approve at Phase 6 start | Phase 6 |
| Project license | Select before public release | Phase 8 |
| `.agents/WORKFLOW.md` rules (commit everything, Copilot trailer, `[CANARY]`) conflict with the owner's global rule of committing only on request | Reconcile before the first code commit | Phase 0 |

## Cross-cutting rules

See [`IMPLEMENTATION-PLAN.md`](IMPLEMENTATION-PLAN.md#cross-cutting-validation-rules): no encoder pin polling, no PDF bytes on the ESP32, atomic upload commits, UTF-8-safe offsets, Spanish UI strings with English code and docs.
