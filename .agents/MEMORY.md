# Agent Memory

> **Strict rule:** This file is append-only. Never delete, reorder, or overwrite an existing line. Add new actions, generated features, and decisions at the end.

## 2026-10-13 — Repository and agent filesystem initialization

- Confirmed the project directory was initially empty and was not a Git repository.
- Initialized a local Git repository on branch `main`.
- Created the private GitHub repository [albertorblan06/leeloo](https://github.com/albertorblan06/leeloo).
- Configured `origin` as `git@github.com:albertorblan06/leeloo.git`.
- Created the `.agents` directory and its operational files: `CONTEXT.md`, `MEMORY.md`, `WORKFLOW.md`, and `ERRORS.md`.
- Established the first repository commit separately from the `.agents` filesystem commit.

## 2026-10-13 — Leeloo architecture discovery started

- Confirmed the repository has no application or hardware implementation yet.
- Preserved the complete technical, functional, and edge-case questionnaire in `docs/ARCHITECTURE-QUESTIONS.md`.
- Added an orientative `README.md` describing the product concept and major system areas without locking implementation choices.
- Recorded that architecture selection is gated on answers covering hardware, display, input, RSVP semantics, browser conversion, storage, power, security, reliability, and validation.

## 2026-10-14 — Product architecture baseline

- Incorporated the user-supplied answers from `docs/ARCHITECTURE-ANSWERS.md`.
- Selected Arduino-ESP32 with underlying FreeRTOS for the firmware baseline.
- Defined a Spanish device UI and English repository/code documentation.
- Defined minimal live RSVP telemetry (battery and WPM), expanded paused telemetry, strict word-based RSVP, and byte-offset progress persistence.
- Defined the single UTF-8 metadata-header book format and mathematical paused page estimate without sidecar files or inline page markers.
- Added `docs/ARCHITECTURE.md` and `docs/IMPLEMENTATION-PLAN.md` with architecture boundaries, assumptions, milestones, and exit evidence.
- Added `.gitignore` to exclude `.DS_Store`.

## 2026-10-14 — OLED display specification revised

- Updated the display target from 0.96-inch to 1.3-inch monochrome 128x64 over I2C.
- Recorded SH1106 as the likely but unverified controller.
- Kept controller and module voltage/pin verification as a hardware purchase and driver-selection gate.
- Updated the requirements answers, architecture, implementation plan, README, and agent context to match.

## 2026-10-20 — Roadmap and unit-testing policy

- Added `docs/ROADMAP.md` with nine phases mapped to the implementation-plan milestones, each with tasks, risks and a done criterion.
- Added project `CLAUDE.md` defining the unit-testing policy: PlatformIO `native` + Unity for firmware, Vitest for the web app (pending dependency approval), 90 % / 80 % coverage targets.
- Recorded open decisions in the roadmap: PlatformIO vs arduino-cli, web dependencies, project license, and the conflict between `.agents/WORKFLOW.md` commit rules and the owner's global rules.

## 2026-10-20 — Phase 0 scaffold

- Added `platformio.ini` (`native` + provisional `esp32s3` on `platformio/espressif32@7.1.3`), `lib/leeloo_core/` (UTF-8 sequence length helper), `test/test_smoke/`, minimal `src/main.cpp`, `.clang-format`, `.gitignore` entries and `.github/workflows/ci.yml`.
- Verified locally: `pio test -e native` passes (4 tests) and fails when an assertion is broken; `pio run -e esp32s3` builds. CI has not run on GitHub yet.
- Deliberately not set: flash size, PSRAM, partitions and core version (hardware unverified). Coverage and format enforcement deferred to Phase 5 (`gcovr`, `clang-format` not installed).

## 2026-10-20 — Ambient light sensor and haptic feedback added to the architecture

- Added a digital I2C ambient light sensor (Auto/Manual brightness with smoothing and hysteresis) and an I2C haptic driver (cues on WPM step and, optionally, punctuation) to `docs/ARCHITECTURE.md`, the implementation plan, the roadmap, the requirements addendum, README and context.
- All new parts, I2C addresses, motor type, current budget and pins are explicitly unverified until the Phase 1 hardware gate. Punctuation haptics are optional; their default is decided after a feel test.
- Documentation only; no code or build changes.

## 2026-11-04 — OLED RSVP smoke test (branch `dev`)

- Created branch `dev` for hardware experiments; `main` is untouched until work is promoted.
- Added `lib/rsvp/` (`next_word`, `focal_split`, `word_delay_ms`) with 21 Unity tests in `test/test_rsvp/`; `pio test -e native` passes (25 tests total).
- `src/main.cpp` now drives the OLED through U8g2 `2.36.18` (pinned in `[env:esp32s3]` only; user approved the dependency): I2C SDA=GPIO8, SCL=GPIO9, scan for 0x3C/0x3D, hardcoded phrase looping at 300 WPM with the focal letter on x=64. `pio run -e esp32s3` builds without warnings.
- NOT verified on hardware: controller is still assumed SH1106, I2C address and visuals are unchecked. Use `docs/hardware-checklists/oled-rsvp-smoke.md` and record the real controller here afterwards.
- Coverage of `lib/rsvp` not measured (`gcovr` not installed, deferred to Phase 5).

## 2026-11-04 — Focal accent and narrow fonts for the OLED RSVP smoke test

- Replaced the two center marks with an inverted focal letter (white box, black letter) drawn in `src/main.cpp`; box padding is `kBoxPadX`/`kBoxPadY` for tuning on hardware.
- Font chain is now `helvB18` → `helvB14` → `7x14B` → `6x13B`. Long words fit 128 px instead of shrinking to 8 pt. The smallest font is 13 px tall, below the 14 pt target the user wanted; accepted because it is the only one that fits 19 letters.
- Speed lowered to 200 WPM after the first hardware run felt too fast.
- Visual result is verified only by the user on the board; `pio test -e native` (29) and `pio run -e esp32s3` pass.

## 2026-11-04 — Syllable split and outline focal letter (OLED RSVP)

- Reverted the inverted box and the narrow font chain: one font, `helvB18_tf`, with the two center marks restored and a 1 px outline only around the focal letter (`kFramePadX/Y` for tuning).
- Long words are split at a syllable boundary with a trailing hyphen (`lib/rsvp` `split_point`, 11 tests). The split is recursive so a piece that still does not fit is split again. Each piece takes one beat.
- Deviation from the approved plan: a word with no syllable boundary (e.g. `pan`) returns 0 and is shown clipped, not cut in the middle.
- `split_point` is an approximation of Spanish syllabification (no prefixes, loanwords, or rare hiatus exceptions). Visual result is validated only by the user on the board.

## 2026-11-04 — Pauses for punctuation, paragraphs and long words (RSVP)

- `lib/rsvp` `hold_percent` (percent of one word period, 100 = normal): trailing comma-level marks 200, sentence-level 300, paragraph (two or more newlines after the word) 500, words of more than 8 letters 125. The largest applies. Opening marks (¿ ¡ ( « —) never pause. Tests in `test/test_rsvp/` (57 total).
- Firmware: the last piece of a word carries its hold; earlier pieces of a split word get a normal beat. Paragraph test text has a blank line after "fallo.".
- Known limit: abbreviations such as "Sr." count as sentence ends (`shortcut:` in `rsvp.cpp`).
- Pause durations are chosen by the user ("Marcado" preset); whether they feel right is validated only on the board.
