# Agent Context

This file is the primary source of truth for the agent regarding the current state of the project.

## Project State

- **Project:** `leeloo`
- **Repository:** [albertorblan06/leeloo](https://github.com/albertorblan06/leeloo)
- **Visibility:** Public
- **Default branch:** `main`
- **Remote:** `origin` uses SSH: `git@github.com:albertorblan06/leeloo.git`
- **Project root:** The repository root contains all project files. Agent operational files are maintained in `.agents/`.
- **Product phase:** Leeloo has a documented architecture baseline and implementation roadmap; hardware purchase and firmware implementation are gated by board/display/power validation.
- **Product baseline:** Requirements answers are recorded in `docs/ARCHITECTURE-ANSWERS.md`; the normalized architecture is in `docs/ARCHITECTURE.md`; the implementation milestones are in `docs/IMPLEMENTATION-PLAN.md`; the phased roadmap is in `docs/ROADMAP.md`; the unit-testing policy is in the root `CLAUDE.md`.
- **Display:** 1.3-inch monochrome 128x64 OLED over I2C; SH1106 is likely but unverified pending exact module identification.
- **Planning documents:** The requirements questionnaire is in `docs/ARCHITECTURE-QUESTIONS.md`; the project overview is in `README.md`.

## Agent Records

- Historical actions, generated features, and decisions: [MEMORY.md](MEMORY.md)
- Failed approaches, errors, and lessons learned: [ERRORS.md](ERRORS.md)
- Operational rules and file architecture: [WORKFLOW.md](WORKFLOW.md)

## Maintenance

Update this file when the current project state or the location and purpose of project components changes. Preserve historical details in `MEMORY.md` rather than replacing them here.
