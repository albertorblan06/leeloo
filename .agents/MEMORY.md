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
