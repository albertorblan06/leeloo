# Graph Report - leeloo  (2026-11-04)

## Corpus Check
- 19 files · ~12,229 words
- Verdict: corpus is large enough that graph structure adds value.
- Unclassified: 3 file(s) not represented in the graph (top: (none) 2, .ini 1)

## Summary
- 208 nodes · 374 edges · 10 communities (9 shown, 1 thin omitted)
- Extraction: 86% EXTRACTED · 14% INFERRED · 0% AMBIGUOUS · INFERRED: 54 edges (avg confidence: 0.88)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `68d07446`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- rsvp.h
- Agent Memory and Workflow
- Core C++ Scaffold
- Delivery Phases and Coverage
- RSVP Engine and Storage
- test_rsvp.cpp
- main.cpp
- rsvp.cpp
- Leeloo Architecture Questionnaire
- OLED RSVP smoke test

## God Nodes (most connected - your core abstractions)
1. `Leeloo Roadmap` - 19 edges
2. `Leeloo Product Architecture` - 17 edges
3. `Leeloo Implementation Plan` - 16 edges
4. `Agent Memory (MEMORY.md)` - 14 edges
5. `Architecture Answers (requirements)` - 14 edges
6. `Project file architecture table` - 13 edges
7. `Agent Context (CONTEXT.md)` - 12 edges
8. `Leeloo Architecture Questionnaire` - 11 edges
9. `Haptic feedback` - 11 edges
10. `Project CLAUDE.md` - 10 edges

## Surprising Connections (you probably didn't know these)
- `Pure logic in lib/ behind thin hardware interfaces` --semantically_similar_to--> `Test strategy`  [INFERRED] [semantically similar]
  CLAUDE.md → docs/ROADMAP.md
- `Unit-testing policy` --semantically_similar_to--> `Cross-cutting validation rules`  [INFERRED] [semantically similar]
  CLAUDE.md → docs/IMPLEMENTATION-PLAN.md
- `Unit-testing policy` --semantically_similar_to--> `Test strategy`  [INFERRED] [semantically similar]
  CLAUDE.md → docs/ROADMAP.md
- `Reliability and diagnostics` --semantically_similar_to--> `Unit-testing policy`  [INFERRED] [semantically similar]
  docs/ARCHITECTURE.md → CLAUDE.md
- `Hardware purchase gated by board/display/power validation` --conceptually_related_to--> `Phase 1 - Hardware validation gates`  [INFERRED]
  .agents/CONTEXT.md → docs/ROADMAP.md

## Import Cycles
- None detected.

## Hyperedges (group relationships)
- **I2C peripherals sharing one bus** — docs_architecture_oled_display, docs_architecture_ambient_light_sensor, docs_architecture_haptic_feedback, docs_architecture_i2c_bus_owner [EXTRACTED 1.00]
- **Requirements to architecture to plan to roadmap chain** — docs_architecture_answers, docs_architecture, docs_implementation_plan, docs_roadmap [EXTRACTED 1.00]
- **Phone to device book transfer pipeline** — docs_architecture_browser_conversion, docs_architecture_wifi_session, docs_architecture_book_format, docs_architecture_littlefs_storage, docs_architecture_rsvp_behavior [INFERRED 0.85]

## Communities (10 total, 1 thin omitted)

### Community 0 - "rsvp.h"
Cohesion: 0.22
Nodes (6): FocalSplit, focal_bytes, prefix_bytes, WordSpan, length, start

### Community 1 - "Agent Memory and Workflow"
Cohesion: 0.14
Nodes (41): Agent Context (CONTEXT.md), Hardware purchase gated by board/display/power validation, Project State, Agent Errors and Lessons, Agent Memory (MEMORY.md), Append-only memory rule, Agent Workflow (WORKFLOW.md), Canary Flag (+33 more)

### Community 3 - "Delivery Phases and Coverage"
Cohesion: 0.31
Nodes (10): Phase 0 scaffold (memory entry), Coverage targets (90% firmware, 80% web), Milestone 1: Project and toolchain foundation, CI blocked by GitHub billing issue (TODO), Phase 0 - Repo and test foundation, Phase 5 - Retrofit tests and CI hardening, CI workflow (ci.yml), Build esp32s3 step (compile-only) (+2 more)

### Community 4 - "RSVP Engine and Storage"
Cohesion: 0.21
Nodes (14): Required edge cases, MVP definition (phone uploads PDF, converts to txt, ESP32 renders RSVP), Single-file UTF-8 book format, Browser-side PDF conversion, LittleFS storage and atomic writes, Explicit non-goals, RSVP behavior (150-800 WPM, word tokens, byte-offset anchor), Wi-Fi soft AP session with PIN (+6 more)

### Community 5 - "test_rsvp.cpp"
Cohesion: 0.05
Nodes (3): test_next_word_skips_leading_spaces(), test_next_word_treats_tabs_and_newlines_as_separators(), word_at()

### Community 7 - "main.cpp"
Cohesion: 0.14
Nodes (20): build_text(), copy_slice(), draw_at(), draw_piece(), fits(), i2c_device_present(), Layout, fits (+12 more)

### Community 8 - "rsvp.cpp"
Cohesion: 0.12
Nodes (26): BestSplit, byte, cost, letters, classify(), count_letters(), decode_at(), focal_split() (+18 more)

### Community 9 - "Leeloo Architecture Questionnaire"
Cohesion: 0.18
Nodes (11): Decisions to lock before implementation, Display and visual design, ESP32-S3 and development environment, Input and interaction, Leeloo Architecture Questionnaire, Power, time, and physical product, Product and prototype scope, Reliability, security, and development (+3 more)

### Community 10 - "OLED RSVP smoke test"
Cohesion: 0.40
Nodes (4): Checks, OLED RSVP smoke test, Run log, Setup

## Knowledge Gaps
- **23 isolated node(s):** `kOther`, `kConsonant`, `kStrongVowel`, `kWeakVowel`, `byte` (+18 more)
  These have ≤1 connection - possible missing edges or undocumented components. (Counts symbols only; 89 node(s) total have ≤1 connection when file, concept and rationale nodes are included.)
- **1 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `Leeloo Architecture Questionnaire` connect `Leeloo Architecture Questionnaire` to `Agent Memory and Workflow`?**
  _High betweenness centrality (0.033) - this node is a cross-community bridge._
- **Are the 2 inferred relationships involving `Agent Memory (MEMORY.md)` (e.g. with `Single-file UTF-8 book format` and `Arduino-ESP32 with FreeRTOS firmware`) actually correct?**
  _`Agent Memory (MEMORY.md)` has 2 INFERRED edges - model-reasoned connections that need verification._
- **What connects `kOther`, `kConsonant`, `kStrongVowel` to the rest of the system?**
  _23 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `Agent Memory and Workflow` be split into smaller, more focused modules?**
  _Cohesion score 0.13588850174216027 - nodes in this community are weakly interconnected._
- **Why does `Leeloo Roadmap` connect `Agent Memory and Workflow` to `Delivery Phases and Coverage`, `RSVP Engine and Storage`?**
  _High betweenness centrality (0.032) - this node is a cross-community bridge._
- **Are the 8 inferred relationships involving `Architecture Answers (requirements)` (e.g. with `Single-file UTF-8 book format` and `EC11 rotary encoder input`) actually correct?**
  _`Architecture Answers (requirements)` has 8 INFERRED edges - model-reasoned connections that need verification._
- **Should `Core C++ Scaffold` be split into smaller, more focused modules?**
  _Cohesion score 0.14285714285714285 - nodes in this community are weakly interconnected._