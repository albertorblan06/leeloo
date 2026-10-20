# Agent Workflow

## Commit Policy

The agent must commit all changes, no matter how minor. Commits should describe the change clearly and must include the required Copilot co-author trailer:

`Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>`

## Project Architecture

All project files are located under the repository root. The `.agents/` directory is the agent's operational filesystem and is kept under version control.

| Path | Purpose |
| --- | --- |
| `.agents/CONTEXT.md` | Primary source of truth for the current project state and links to the agent's historical records. |
| `.agents/MEMORY.md` | Strict append-only history of agent actions, generated features, and decisions. |
| `.agents/WORKFLOW.md` | Operational rules, commit policy, architecture, and this file's maintenance boundary. |
| `.agents/ERRORS.md` | Append-only post-mortem log of errors, bugs, failed approaches, and lessons to prevent recurrence. |
| `docs/ARCHITECTURE-QUESTIONS.md` | Original requirements questionnaire used for product discovery. |
| `docs/ARCHITECTURE-ANSWERS.md` | User-supplied answers and product decisions, preserved as the requirements source. |
| `docs/ARCHITECTURE.md` | Normalized product and technical architecture baseline. |
| `docs/IMPLEMENTATION-PLAN.md` | Milestones, exit evidence, and cross-cutting validation rules. |
| `platformio.ini` | PlatformIO environments: `native` (host unit tests) and provisional `esp32s3`. |
| `lib/` | Hardware-free C++ modules (e.g. `lib/leeloo_core/`). |
| `test/` | Unity unit tests run with `pio test -e native`. |
| `src/` | Arduino firmware entry point (`src/main.cpp`). |
| `.github/workflows/ci.yml` | CI: host tests and ESP32 compile check. |
| `.clang-format` | Formatting config (not enforced yet). |
| `docs/ROADMAP.md` | Phased roadmap with objectives, tasks, risks, done criteria and open decisions. |
| `CLAUDE.md` | Project instructions for Claude Code, including the unit-testing policy. |
| `README.md` | Public project overview and navigation to architecture documents. |
| `.gitignore` | Repository hygiene rules for local-only files such as `.DS_Store`. |
| Repository root | Location for all application, configuration, documentation, and other project files created for `leeloo`. |
| `.git/` | Local Git metadata, history, branches, and remote tracking configuration. |

## Canary Flag

The agent MUST output the word `[CANARY]` in every single response. This tracks context continuity and signals when a new session may be required.

## File Maintenance Rules

- `CONTEXT.md` is updated when the current project state or architecture changes.
- `MEMORY.md` is strictly append-only. Never delete or overwrite a line.
- `ERRORS.md` is strictly append-only. Record every error, bug, or failed approach with its lesson.
- `WORKFLOW.md` may only be modified to document changes in the file architecture or to explain the purpose of newly created files.
