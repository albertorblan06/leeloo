# Graph Report - leeloo  (2026-11-05)

## Corpus Check
- 77 files · ~41,164 words
- Verdict: corpus is large enough that graph structure adds value.
- Unclassified: 5 file(s) not represented in the graph (top: (none) 2, .csv 1, .ini 1)

## Summary
- 1077 nodes · 2140 edges · 49 communities (45 shown, 4 thin omitted)
- Extraction: 88% EXTRACTED · 12% INFERRED · 0% AMBIGUOUS · INFERRED: 260 edges (avg confidence: 0.86)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `7600d867`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- test_joystick.cpp
- Leeloo Roadmap
- BookValidator
- hold_of
- test_buttons.cpp
- word_at
- Temporary input: analog joystick (replaces the 3 buttons)
- rsvp.cpp
- Leeloo Architecture Questionnaire
- OLED RSVP smoke test
- Temporary input: 3 push buttons (replaces the EC11 encoder)
- test_menu.cpp
- app.js
- upload_server.cpp
- package.json
- Leeloo Design Guide
- Upload web API contract
- test_upload.cpp
- Result
- percent_decode
- upload.cpp
- sanitize_name
- check_capacity
- build.mjs
- Ui
- rsvp_view.cpp
- test_ui.cpp
- test_text.cpp
- test_settings.cpp
- test_inactivity.cpp
- storage_books.cpp
- main.cpp
- test_reader.cpp
- Ui
- RSVP reading on hardware
- Leeloo Device Menu Design
- On-device menu (joystick)
- settings.h
- Reader
- Wi-Fi soft AP and book upload
- handle_button_level
- Command
- format_pin
- Press
- ends_first_sentence
- settings_store.cpp
- AppState

## God Nodes (most connected - your core abstractions)
1. `press()` - 50 edges
2. `make_ui()` - 49 edges
3. `Ui` - 46 edges
4. `open_root_item()` - 33 edges
5. `Reader` - 32 edges
6. `validate()` - 28 edges
7. `shown()` - 23 edges
8. `Event` - 22 edges
9. `Result` - 22 edges
10. `Leeloo Roadmap` - 19 edges

## Surprising Connections (you probably didn't know these)
- `Pure logic in lib/ behind thin hardware interfaces` --semantically_similar_to--> `Test strategy`  [INFERRED] [semantically similar]
  CLAUDE.md → docs/ROADMAP.md
- `Unit-testing policy` --semantically_similar_to--> `Cross-cutting validation rules`  [INFERRED] [semantically similar]
  CLAUDE.md → docs/IMPLEMENTATION-PLAN.md
- `Unit-testing policy` --semantically_similar_to--> `Test strategy`  [INFERRED] [semantically similar]
  CLAUDE.md → docs/ROADMAP.md
- `Reliability and diagnostics` --semantically_similar_to--> `Unit-testing policy`  [INFERRED] [semantically similar]
  docs/ARCHITECTURE.md → CLAUDE.md
- `test_body_offset_is_nullopt_for_empty_input()` --calls--> `body_offset()`  [INFERRED]
  test/test_reader/test_reader.cpp → lib/reader/reader.cpp

## Import Cycles
- None detected.

## Hyperedges (group relationships)
- **I2C peripherals sharing one bus** — docs_architecture_oled_display, docs_architecture_ambient_light_sensor, docs_architecture_haptic_feedback, docs_architecture_i2c_bus_owner [EXTRACTED 1.00]
- **Requirements to architecture to plan to roadmap chain** — docs_architecture_answers, docs_architecture, docs_implementation_plan, docs_roadmap [EXTRACTED 1.00]
- **Phone to device book transfer pipeline** — docs_architecture_browser_conversion, docs_architecture_wifi_session, docs_architecture_book_format, docs_architecture_littlefs_storage, docs_architecture_rsvp_behavior [INFERRED 0.85]

## Communities (49 total, 4 thin omitted)

### Community 0 - "test_joystick.cpp"
Cohesion: 0.13
Nodes (15): Axis, center_, state_, Direction, High, Low, None, test_hysteresis_does_not_retrigger_between_exit_and_enter() (+7 more)

### Community 1 - "Leeloo Roadmap"
Cohesion: 0.08
Nodes (65): Agent Context (CONTEXT.md), Hardware purchase gated by board/display/power validation, Project State, Agent Errors and Lessons, Agent Memory (MEMORY.md), Append-only memory rule, Phase 0 scaffold (memory entry), Agent Workflow (WORKFLOW.md) (+57 more)

### Community 2 - "BookValidator"
Cohesion: 0.13
Nodes (14): BookValidator, body_bytes_, error_, line_, line_len_, pages_seen_, state_, total_ (+6 more)

### Community 3 - "hold_of"
Cohesion: 0.12
Nodes (16): hold_of(), test_hold_adds_bonus_for_long_words_counting_codepoints_not_bytes(), test_hold_handles_digits_and_decimals(), test_hold_handles_punctuation_only_token(), test_hold_ignores_carriage_returns_in_paragraph_gap(), test_hold_ignores_opening_marks_at_word_start(), test_hold_is_comma_level_for_closing_quote_paren_and_dash(), test_hold_is_comma_level_for_comma_semicolon_colon() (+8 more)

### Community 4 - "test_buttons.cpp"
Cohesion: 0.10
Nodes (18): classify(), Debouncer, changed_at_, raw_high_, stable_high_, Edge, None, Pressed (+10 more)

### Community 6 - "word_at"
Cohesion: 0.40
Nodes (3): test_next_word_skips_leading_spaces(), test_next_word_treats_tabs_and_newlines_as_separators(), word_at()

### Community 7 - "Temporary input: analog joystick (replaces the 3 buttons)"
Cohesion: 0.40
Nodes (4): Behavior, Checks, Temporary input: analog joystick (replaces the 3 buttons), Wiring

### Community 8 - "rsvp.cpp"
Cohesion: 0.10
Nodes (34): BestSplit, byte, cost, letters, classify(), count_letters(), count_newlines_after(), decode_at() (+26 more)

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
Cohesion: 0.09
Nodes (7): utf8_sequence_length(), Menu, count_, test_down_wraps_from_last_to_first(), test_empty_menu_ignores_input(), test_select_returns_current_index(), test_up_wraps_from_first_to_last()

### Community 13 - "app.js"
Cohesion: 0.08
Nodes (27): vitest, busy(), loadPdfjs(), show(), steps, ApiError, authenticate(), errorFromResponse() (+19 more)

### Community 14 - "upload_server.cpp"
Cohesion: 0.12
Nodes (18): auth_handler(), authorized(), book_name_from_request(), books_handler(), drain_body(), fail_with(), last_activity_ms(), make_route() (+10 more)

### Community 15 - "package.json"
Cohesion: 0.11
Nodes (17): pdfjs-dist, author, dependencies, pdfjs-dist, description, devDependencies, vitest, keywords (+9 more)

### Community 16 - "Leeloo Design Guide"
Cohesion: 0.11
Nodes (18): 10. Source notes, 1. Principles, 2. Web app tokens (measured on Loudop), 3.1 Grid, 3.2 Hierarchy, 3.3 Scrolling and wrapping, 3.4 Motion, 3. OLED rules (128x64, monochrome) (+10 more)

### Community 17 - "Upload web API contract"
Cohesion: 0.20
Nodes (9): 1. Session lifecycle, 2. Static assets, 3.1 `POST /api/auth`, 3.2 `POST /api/books`, 3. Endpoints, 4. Limits, 5. Error body, 6. Open points (+1 more)

### Community 18 - "test_upload.cpp"
Cohesion: 0.14
Nodes (26): test_accepts_book_exactly_at_size_limit(), test_accepts_book_with_pages(), test_accepts_header_line_at_limit(), test_accepts_minimal_valid_book(), test_accepts_spanish_multibyte_content(), test_bytewise_feed_matches_single_chunk(), test_rejects_author_before_title(), test_rejects_book_one_byte_over_size_limit() (+18 more)

### Community 19 - "Result"
Cohesion: 0.15
Nodes (14): error_code(), http_status(), Result, BadName, InvalidFormat, InvalidUtf8, LibraryFull, NoSpace (+6 more)

### Community 20 - "percent_decode"
Cohesion: 0.15
Nodes (12): extract_pin(), hex_value(), percent_decode(), pin_matches(), test_extract_pin_reads_the_web_app_body(), test_extract_pin_rejects_malformed_bodies(), test_extract_pin_tolerates_whitespace(), test_percent_decode_decodes_utf8_escapes() (+4 more)

### Community 21 - "upload.cpp"
Cohesion: 0.32
Nodes (5): has_value(), is_forbidden_name_byte(), is_pages_line(), starts_with(), test_first_error_is_sticky()

### Community 22 - "sanitize_name"
Cohesion: 0.12
Nodes (14): sanitize_name(), Utf8Checker, hi_, lo_, need_, test_sanitize_accepts_name_at_byte_limit(), test_sanitize_accepts_spanish_name(), test_sanitize_counts_bytes_not_characters() (+6 more)

### Community 23 - "check_capacity"
Cohesion: 0.29
Nodes (7): check_capacity(), test_capacity_accepts_last_free_slot(), test_capacity_does_not_overflow_with_huge_book(), test_capacity_ok_with_room(), test_capacity_rejects_full_library(), test_capacity_rejects_when_free_below_reserve(), test_capacity_rejects_when_reserve_would_be_used()

### Community 25 - "Ui"
Cohesion: 0.11
Nodes (37): book_label(), draw_about(), draw_brightness(), draw_contrast(), draw_delete_confirm(), draw_delete_list(), draw_gutter(), draw_line() (+29 more)

### Community 26 - "rsvp_view.cpp"
Cohesion: 0.16
Nodes (22): clip(), collect(), copy_slice(), draw_at(), draw_layout(), draw_piece(), fits(), Layout (+14 more)

### Community 27 - "test_ui.cpp"
Cohesion: 0.13
Nodes (52): settings_, make_ui(), open_delete_list(), open_root_item(), open_setting(), press(), test_about_is_read_only(), test_auto_brightness_reports_missing_sensor() (+44 more)

### Community 28 - "test_text.cpp"
Cohesion: 0.11
Nodes (22): count_chars(), is_continuation(), offset_after_chars(), scroll_start(), truncate_chars(), valid_utf8(), test_counts_spanish_letters_as_one_character(), test_empty_text_stays_empty() (+14 more)

### Community 29 - "test_settings.cpp"
Cohesion: 0.11
Nodes (20): cycle_brightness(), cycle_wifi(), display_register(), sanitize(), snap_percent(), step_percent(), test_brightness_mode_toggles(), test_register_is_full_scale_at_hundred_percent() (+12 more)

### Community 30 - "test_inactivity.cpp"
Cohesion: 0.16
Nodes (10): InactivityTimer, last_ms_, limit_ms_, running_, test_activity_restarts_the_window(), test_does_not_expire_before_the_limit(), test_expires_exactly_at_the_limit(), test_stop_disarms_the_timer() (+2 more)

### Community 31 - "storage_books.cpp"
Cohesion: 0.11
Nodes (20): base_name(), book_count(), book_path_for(), BookText, data, size, find_book(), free_bytes() (+12 more)

### Community 32 - "main.cpp"
Cohesion: 0.19
Nodes (15): check_inactivity(), close_book(), dispatch(), display_status(), end_upload_session(), execute(), finish_book(), loop() (+7 more)

### Community 33 - "test_reader.cpp"
Cohesion: 0.06
Nodes (26): body_offset(), shown(), test_body_offset_is_nullopt_for_empty_input(), test_body_offset_is_nullopt_without_start_marker(), test_body_offset_points_after_start_marker_line(), test_last_piece_keeps_the_punctuation_hold(), test_offset_inside_utf8_sequence_restarts_at_first_word(), test_offset_past_end_restarts_at_first_word() (+18 more)

### Community 34 - "Ui"
Cohesion: 0.05
Nodes (42): Event, Back, BookOpenFailed, DeleteFailed, Down, Left, ReaderFinished, Right (+34 more)

### Community 35 - "RSVP reading on hardware"
Cohesion: 0.25
Nodes (7): Before flashing, Book opening, Pause and sentence navigation, Playback, Resume and end, RSVP reading on hardware, Storage

### Community 36 - "Leeloo Device Menu Design"
Cohesion: 0.18
Nodes (11): 1. Goals and constraints, 2. Menu hierarchy, 3. Navigation and feedback, 4. OLED layout, 5. Open hardware checks, 6. Implementation decisions (first firmware pass), Ajustes, Biblioteca (+3 more)

### Community 37 - "On-device menu (joystick)"
Cohesion: 0.18
Nodes (10): Ajustes (persistence), Before flashing, Biblioteca and Wi-Fi, Boot, Info, Known limits to watch, Leer and Borrar, Measurements (+2 more)

### Community 38 - "settings.h"
Cohesion: 0.16
Nodes (12): BrightnessMode, Auto, Manual, Settings, brightness_mode, brightness_percent, contrast_percent, wifi (+4 more)

### Community 39 - "Reader"
Cohesion: 0.09
Nodes (18): is_continuation(), is_space(), previous_word_start(), Reader, beat_ms_, current_, due_ms_, pieces_ (+10 more)

### Community 40 - "Wi-Fi soft AP and book upload"
Cohesion: 0.25
Nodes (7): Before you start (blockers found while writing this), Conversion in the phone, Measurements to record, PIN and session, Session start, Upload and storage, Wi-Fi soft AP and book upload

### Community 41 - "handle_button_level"
Cohesion: 0.18
Nodes (11): button_task(), ButtonState, debouncer, pressed_at, handle_button_level(), on_book_saved(), post(), RawEdge (+3 more)

### Community 42 - "Command"
Cohesion: 0.18
Nodes (11): Command, CloseBook, DeleteBook, None, OpenBook, ReadingDown, ReadingSelect, ReadingUp (+3 more)

### Community 44 - "format_pin"
Cohesion: 0.50
Nodes (3): format_pin(), test_format_pin_pads_with_zeros(), test_format_pin_wraps_modulo_ten_thousand()

### Community 45 - "Press"
Cohesion: 0.25
Nodes (8): Press, Long, Short, Button, kDown, kSelect, kUp, to_event()

### Community 46 - "ends_first_sentence"
Cohesion: 0.25
Nodes (8): ends_first_sentence(), test_ends_sentence_after_closing_quote_and_period(), test_ends_sentence_for_ellipsis(), test_ends_sentence_for_paragraph_break_without_period(), test_ends_sentence_for_period(), test_ends_sentence_for_question_and_exclamation(), test_ends_sentence_is_false_for_comma(), test_ends_sentence_is_false_for_plain_word()

### Community 48 - "AppState"
Cohesion: 0.40
Nodes (5): AppState, applied_wifi, auto_session, stop_at_ms, timer

## Knowledge Gaps
- **151 isolated node(s):** `None`, `Pressed`, `Released`, `Short`, `Long` (+146 more)
  These have ≤1 connection - possible missing edges or undocumented components. (Counts symbols only; 405 node(s) total have ≤1 connection when file, concept and rationale nodes are included.)
- **4 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `Ui` connect `Ui` to `test_ui.cpp`, `Ui`, `ui.h`, `test_menu.cpp`?**
  _High betweenness centrality (0.038) - this node is a cross-community bridge._
- **What connects `None`, `Pressed`, `Released` to the rest of the system?**
  _151 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `test_joystick.cpp` be split into smaller, more focused modules?**
  _Cohesion score 0.13043478260869565 - nodes in this community are weakly interconnected._
- **Why does `Event` connect `Ui` to `main.cpp`, `handle_button_level`, `ui.h`, `Press`, `test_ui.cpp`?**
  _High betweenness centrality (0.035) - this node is a cross-community bridge._
- **Should `Leeloo Roadmap` be split into smaller, more focused modules?**
  _Cohesion score 0.0781387181738367 - nodes in this community are weakly interconnected._
- **Why does `Reader` connect `Reader` to `ui.h`?**
  _High betweenness centrality (0.028) - this node is a cross-community bridge._
- **Should `BookValidator` be split into smaller, more focused modules?**
  _Cohesion score 0.13333333333333333 - nodes in this community are weakly interconnected._