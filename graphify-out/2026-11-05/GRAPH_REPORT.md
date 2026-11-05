# Graph Report - leeloo  (2026-11-05)

## Corpus Check
- 38 files · ~18,881 words
- Verdict: corpus is large enough that graph structure adds value.
- Unclassified: 3 file(s) not represented in the graph (top: (none) 2, .ini 1)

## Summary
- 407 nodes · 668 edges · 18 communities (16 shown, 2 thin omitted)
- Extraction: 87% EXTRACTED · 13% INFERRED · 0% AMBIGUOUS · INFERRED: 85 edges (avg confidence: 0.87)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `f78b1b9b`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- test_joystick.cpp
- Leeloo Roadmap
- Core C++ Scaffold
- hold_of
- test_buttons.cpp
- word_at
- main.cpp
- rsvp.cpp
- Leeloo Architecture Questionnaire
- OLED RSVP smoke test
- Temporary input: 3 push buttons (replaces the EC11 encoder)
- test_menu.cpp
- CI workflow (ci.yml)
- Temporary input: analog joystick (replaces the 3 buttons)
- package.json
- Leeloo Design Guide
- Upload web API contract

## God Nodes (most connected - your core abstractions)
1. `Leeloo Roadmap` - 19 edges
2. `Ui` - 17 edges
3. `hold_of()` - 17 edges
4. `Leeloo Product Architecture` - 17 edges
5. `Leeloo Implementation Plan` - 16 edges
6. `Agent Memory (MEMORY.md)` - 14 edges
7. `Architecture Answers (requirements)` - 14 edges
8. `Project file architecture table` - 13 edges
9. `Agent Context (CONTEXT.md)` - 12 edges
10. `Leeloo Architecture Questionnaire` - 11 edges

## Surprising Connections (you probably didn't know these)
- `Behavior` --references--> `axis_task()`  [INFERRED]
  docs/hardware-checklists/joystick-input.md → src/main.cpp
- `Pure logic in lib/ behind thin hardware interfaces` --semantically_similar_to--> `Test strategy`  [INFERRED] [semantically similar]
  CLAUDE.md → docs/ROADMAP.md
- `Unit-testing policy` --semantically_similar_to--> `Cross-cutting validation rules`  [INFERRED] [semantically similar]
  CLAUDE.md → docs/IMPLEMENTATION-PLAN.md
- `Unit-testing policy` --semantically_similar_to--> `Test strategy`  [INFERRED] [semantically similar]
  CLAUDE.md → docs/ROADMAP.md
- `Reliability and diagnostics` --semantically_similar_to--> `Unit-testing policy`  [INFERRED] [semantically similar]
  docs/ARCHITECTURE.md → CLAUDE.md

## Import Cycles
- None detected.

## Hyperedges (group relationships)
- **I2C peripherals sharing one bus** — docs_architecture_oled_display, docs_architecture_ambient_light_sensor, docs_architecture_haptic_feedback, docs_architecture_i2c_bus_owner [EXTRACTED 1.00]
- **Requirements to architecture to plan to roadmap chain** — docs_architecture_answers, docs_architecture, docs_implementation_plan, docs_roadmap [EXTRACTED 1.00]
- **Phone to device book transfer pipeline** — docs_architecture_browser_conversion, docs_architecture_wifi_session, docs_architecture_book_format, docs_architecture_littlefs_storage, docs_architecture_rsvp_behavior [INFERRED 0.85]

## Communities (18 total, 2 thin omitted)

### Community 0 - "test_joystick.cpp"
Cohesion: 0.13
Nodes (15): Axis, center_, state_, Direction, High, Low, None, test_hysteresis_does_not_retrigger_between_exit_and_enter() (+7 more)

### Community 1 - "Leeloo Roadmap"
Cohesion: 0.10
Nodes (55): Agent Context (CONTEXT.md), Hardware purchase gated by board/display/power validation, Project State, Agent Errors and Lessons, Agent Memory (MEMORY.md), Append-only memory rule, Agent Workflow (WORKFLOW.md), Canary Flag (+47 more)

### Community 3 - "hold_of"
Cohesion: 0.12
Nodes (16): hold_of(), test_hold_adds_bonus_for_long_words_counting_codepoints_not_bytes(), test_hold_handles_digits_and_decimals(), test_hold_handles_punctuation_only_token(), test_hold_ignores_carriage_returns_in_paragraph_gap(), test_hold_ignores_opening_marks_at_word_start(), test_hold_is_comma_level_for_closing_quote_paren_and_dash(), test_hold_is_comma_level_for_comma_semicolon_colon() (+8 more)

### Community 4 - "test_buttons.cpp"
Cohesion: 0.09
Nodes (23): classify(), Debouncer, changed_at_, raw_high_, stable_high_, Edge, None, Pressed (+15 more)

### Community 6 - "word_at"
Cohesion: 0.40
Nodes (3): test_next_word_skips_leading_spaces(), test_next_word_treats_tabs_and_newlines_as_separators(), word_at()

### Community 7 - "main.cpp"
Cohesion: 0.06
Nodes (40): apply_event(), axis_task(), Button, kDown, kLeft, kRight, kSelect, kUp (+32 more)

### Community 8 - "rsvp.cpp"
Cohesion: 0.09
Nodes (36): BestSplit, byte, cost, letters, classify(), count_letters(), count_newlines_after(), decode_at() (+28 more)

### Community 9 - "Leeloo Architecture Questionnaire"
Cohesion: 0.18
Nodes (11): Decisions to lock before implementation, Display and visual design, ESP32-S3 and development environment, Input and interaction, Leeloo Architecture Questionnaire, Power, time, and physical product, Product and prototype scope, Reliability, security, and development (+3 more)

### Community 10 - "OLED RSVP smoke test"
Cohesion: 0.40
Nodes (4): Checks, OLED RSVP smoke test, Run log, Setup

### Community 11 - "Temporary input: 3 push buttons (replaces the EC11 encoder)"
Cohesion: 0.22
Nodes (8): Checks, Firmware notes (when implemented), Full pin summary, Mapping, Menu test (firmware on `dev`), OLED 1.3" 128x64 (I2C, 4 pins, unchanged), Temporary input: 3 push buttons (replaces the EC11 encoder), Wiring (all connections)

### Community 12 - "test_menu.cpp"
Cohesion: 0.16
Nodes (6): Menu, count_, test_down_wraps_from_last_to_first(), test_empty_menu_ignores_input(), test_select_returns_current_index(), test_up_wraps_from_first_to_last()

### Community 13 - "CI workflow (ci.yml)"
Cohesion: 0.31
Nodes (10): Phase 0 scaffold (memory entry), Coverage targets (90% firmware, 80% web), Milestone 1: Project and toolchain foundation, CI blocked by GitHub billing issue (TODO), Phase 0 - Repo and test foundation, Phase 5 - Retrofit tests and CI hardening, CI workflow (ci.yml), Build esp32s3 step (compile-only) (+2 more)

### Community 14 - "Temporary input: analog joystick (replaces the 3 buttons)"
Cohesion: 0.40
Nodes (4): Behavior, Checks, Temporary input: analog joystick (replaces the 3 buttons), Wiring

### Community 15 - "package.json"
Cohesion: 0.09
Nodes (23): pdfjs-dist, vitest, author, dependencies, pdfjs-dist, description, devDependencies, vitest (+15 more)

### Community 16 - "Leeloo Design Guide"
Cohesion: 0.11
Nodes (18): 10. Source notes, 1. Principles, 2. Web app tokens (measured on Loudop), 3.1 Grid, 3.2 Hierarchy, 3.3 Scrolling and wrapping, 3.4 Motion, 3. OLED rules (128x64, monochrome) (+10 more)

### Community 17 - "Upload web API contract"
Cohesion: 0.20
Nodes (9): 1. Session lifecycle, 2. Static assets, 3.1 `POST /api/auth`, 3.2 `POST /api/books`, 3. Endpoints, 4. Limits, 5. Error body, 6. Open points (+1 more)

## Knowledge Gaps
- **50 isolated node(s):** `None`, `Pressed`, `Released`, `Short`, `Long` (+45 more)
  These have ≤1 connection - possible missing edges or undocumented components. (Counts symbols only; 187 node(s) total have ≤1 connection when file, concept and rationale nodes are included.)
- **2 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **What connects `None`, `Pressed`, `Released` to the rest of the system?**
  _50 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `test_joystick.cpp` be split into smaller, more focused modules?**
  _Cohesion score 0.13043478260869565 - nodes in this community are weakly interconnected._
- **Should `Leeloo Roadmap` be split into smaller, more focused modules?**
  _Cohesion score 0.0987012987012987 - nodes in this community are weakly interconnected._
- **Should `hold_of` be split into smaller, more focused modules?**
  _Cohesion score 0.11764705882352941 - nodes in this community are weakly interconnected._
- **Should `test_buttons.cpp` be split into smaller, more focused modules?**
  _Cohesion score 0.08739495798319327 - nodes in this community are weakly interconnected._
- **Should `test_rsvp.cpp` be split into smaller, more focused modules?**
  _Cohesion score 0.04878048780487805 - nodes in this community are weakly interconnected._
- **Should `main.cpp` be split into smaller, more focused modules?**
  _Cohesion score 0.06352941176470588 - nodes in this community are weakly interconnected._