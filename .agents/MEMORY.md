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
