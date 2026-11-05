# Graph Report - leeloo  (2026-10-20)

## Corpus Check
- Corpus is ~6,773 words - fits in a single context window. You may not need a graph.

## Summary
- 71 nodes · 111 edges · 7 communities
- Extraction: 72% EXTRACTED · 28% INFERRED · 0% AMBIGUOUS · INFERRED: 31 edges (avg confidence: 0.84)
- Token cost: 53,723 input · 0 output

## Community Hubs (Navigation)
- Agent Files and Process
- Firmware Core Design
- Hardware and Milestones
- Testing Policy and Roadmap Phases
- Product Baseline and Early Phases
- Web Upload and MVP Scope
- Workflow Rules and Decisions

## God Nodes (most connected - your core abstractions)
1. `Leeloo Implementation Plan` - 14 edges
2. `Leeloo Roadmap` - 11 edges
3. `Agent Context (CONTEXT.md)` - 10 edges
4. `Leeloo README` - 10 edges
5. `Agent Memory (MEMORY.md)` - 8 edges
6. `Project CLAUDE.md` - 7 edges
7. `Agent Workflow (WORKFLOW.md)` - 6 edges
8. `Leeloo Product Architecture` - 6 edges
9. `RSVP Engine` - 6 edges
10. `Unit-testing policy` - 5 edges

## Surprising Connections (you probably didn't know these)
- `Test strategy` --semantically_similar_to--> `PlatformIO native + Unity, Vitest`  [INFERRED] [semantically similar]
  docs/ROADMAP.md → CLAUDE.md
- `Agent Context (CONTEXT.md)` --references--> `Leeloo Implementation Plan`  [EXTRACTED]
  .agents/CONTEXT.md → docs/IMPLEMENTATION-PLAN.md
- `Phase 1 - Hardware validation gates (M2)` --conceptually_related_to--> `Display: 1.3-inch 128x64 I2C OLED (SH1106 likely)`  [INFERRED]
  docs/ROADMAP.md → .agents/CONTEXT.md
- `Agent Memory (MEMORY.md)` --references--> `Leeloo Implementation Plan`  [EXTRACTED]
  .agents/MEMORY.md → docs/IMPLEMENTATION-PLAN.md
- `Phase 3 - Storage and book service (M4)` --conceptually_related_to--> `Single UTF-8 metadata-header book format`  [INFERRED]
  docs/ROADMAP.md → .agents/MEMORY.md

## Hyperedges (group relationships)
- **Agent operating filesystem** — agents_context, agents_memory, agents_workflow, agents_errors [EXTRACTED 1.00]
- **Unit-testing policy components** — claude_unit_testing_policy, claude_frameworks_and_commands, claude_pure_logic_structure, claude_coverage, claude_required_edge_cases, docs_roadmap_test_strategy [INFERRED 0.85]
- **Upload to Read Pipeline** — docs_architecture_browser_upload_app, docs_architecture_wifi_soft_ap_pin, docs_architecture_book_format, docs_architecture_littlefs_storage, docs_architecture_rsvp_engine [INFERRED 0.85]
- **Encoder Input Chain** — docs_architecture_ec11_encoder, docs_architecture_input_task_isr, docs_architecture_ui_state_machine [EXTRACTED 0.95]
- **Leeloo roadmap phases 0-8** — docs_roadmap_phase_0, docs_roadmap_phase_1, docs_roadmap_phase_2, docs_roadmap_phase_3, docs_roadmap_phase_4, docs_roadmap_phase_5, docs_roadmap_phase_6, docs_roadmap_phase_7, docs_roadmap_phase_8 [EXTRACTED 1.00]
- **Leeloo roadmap phases 0-8** — docs_roadmap_phase_0, docs_roadmap_phase_1, docs_roadmap_phase_2, docs_roadmap_phase_3, docs_roadmap_phase_4, docs_roadmap_phase_5, docs_roadmap_phase_6, docs_roadmap_phase_7, docs_roadmap_phase_8 [EXTRACTED 1.00]
- **Agent operating filesystem** — agents_context, agents_memory, agents_workflow, agents_errors [EXTRACTED 1.00]
- **Unit-testing policy components** — claude_unit_testing_policy, claude_frameworks_and_commands, claude_pure_logic_structure, claude_coverage, claude_required_edge_cases, docs_roadmap_test_strategy [INFERRED 0.85]

## Communities (7 total, 0 thin omitted)

### Community 0 - "Agent Files and Process"
Cohesion: 0.30
Nodes (14): Agent Context (CONTEXT.md), Agent Errors and Lessons, Agent Memory (MEMORY.md), Agent Workflow (WORKFLOW.md), File Maintenance Rules, Project CLAUDE.md, Leeloo Product Architecture, ARCHITECTURE-ANSWERS.md (+6 more)

### Community 1 - "Firmware Core Design"
Cohesion: 0.16
Nodes (14): Arduino-ESP32 with FreeRTOS, Byte-offset Progress Persistence, EC11 Rotary Encoder with Push-Button, ISR-fed FreeRTOS Input Task, Spanish UI, English Repo Language, LittleFS Storage and Atomic Writes, Mathematical Paused Page Estimate, Reliability and Diagnostics (+6 more)

### Community 2 - "Hardware and Milestones"
Cohesion: 0.26
Nodes (12): Deep Sleep and Encoder Wake, ESP32-S3 DevKitC-1 N16R8, 1.3-inch 128x64 I2C OLED (SH1106 likely), LiPo 500mAh + TP4056 Power Subsystem, Leeloo Implementation Plan, Hardware Validation Gates (board, OLED, battery), Milestone 1: Project and Toolchain Foundation, Milestone 2: Hardware Smoke Test (+4 more)

### Community 3 - "Testing Policy and Roadmap Phases"
Cohesion: 0.18
Nodes (11): Coverage targets (90% lib, 80% web), PlatformIO native + Unity, Vitest, Pure logic in lib/ behind IClock/IStorage/IDisplay interfaces, Required edge cases, Unit-testing policy, Phase 0 - Repo and test foundation (M1), Phase 5 - Retrofit tests and CI hardening, Phase 6 - Browser upload application (M6) (+3 more)

### Community 4 - "Product Baseline and Early Phases"
Cohesion: 0.22
Nodes (10): Display: 1.3-inch 128x64 I2C OLED (SH1106 likely), Project State, Product architecture baseline decisions, Arduino-ESP32 with FreeRTOS firmware baseline, Single UTF-8 metadata-header book format, Phase 1 - Hardware validation gates (M2), Phase 2 - Firmware core (M3), Phase 3 - Storage and book service (M4) (+2 more)

### Community 5 - "Web Upload and MVP Scope"
Cohesion: 0.33
Nodes (6): MVP: ESP32-hosted web, phone PDF to TXT, RSVP render, Single-file UTF-8 Metadata-Header Book Format, Browser PDF-to-TXT Upload Web App, Core 0 Network / Core 1 UI Partitioning, Explicit Non-goals (no PDF on ESP32, OTA, BLE, OCR), Temporary Soft AP with Per-session PIN

### Community 6 - "Workflow Rules and Decisions"
Cohesion: 0.67
Nodes (3): Canary Flag, Commit Policy, Decisions needed

## Knowledge Gaps
- **23 isolated node(s):** `Project State`, `Commit Policy`, `Canary Flag`, `File Maintenance Rules`, `Pure logic in lib/ behind IClock/IStorage/IDisplay interfaces` (+18 more)
  These have ≤1 connection - possible missing edges or undocumented components. (Counts symbols only; 23 node(s) total have ≤1 connection when file, concept and rationale nodes are included.)

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `Leeloo Implementation Plan` connect `Hardware and Milestones` to `Agent Files and Process`, `Firmware Core Design`?**
  _High betweenness centrality (0.564) - this node is a cross-community bridge._
- **What connects `Project State`, `Commit Policy`, `Canary Flag` to the rest of the system?**
  _23 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Why does `Leeloo Roadmap` connect `Agent Files and Process` to `Hardware and Milestones`, `Testing Policy and Roadmap Phases`, `Workflow Rules and Decisions`?**
  _High betweenness centrality (0.301) - this node is a cross-community bridge._
- **Why does `Phase 1 - Hardware validation gates (M2)` connect `Product Baseline and Early Phases` to `Testing Policy and Roadmap Phases`?**
  _High betweenness centrality (0.213) - this node is a cross-community bridge._