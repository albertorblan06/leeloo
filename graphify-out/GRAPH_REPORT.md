# Graph Report - leeloo  (2026-11-25)

## Corpus Check
- 107 files · ~56,090 words
- Verdict: corpus is large enough that graph structure adds value.
- Unclassified: 9 file(s) not represented in the graph (top: .stl 4, (none) 2, .csv 1)

## Summary
- 1646 nodes · 3296 edges · 85 communities (75 shown, 10 thin omitted)
- Extraction: 89% EXTRACTED · 11% INFERRED · 0% AMBIGUOUS · INFERRED: 378 edges (avg confidence: 0.85)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `3c175ba8`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- build_enclosure.py
- Leeloo Roadmap
- Menu
- hold_of
- test_buttons.cpp
- word_at
- Temporary input: analog joystick (replaces the 3 buttons)
- rsvp.cpp
- NetService
- OLED RSVP smoke test
- Temporary input: 3 push buttons (replaces the EC11 encoder)
- test_joystick.cpp
- app.js
- upload_server.cpp
- package.json
- Leeloo Design Guide
- Upload web API contract
- test_upload.cpp
- Result
- percent_decode
- upload.cpp
- .take
- sanitize_name
- build.mjs
- Ui
- rsvp_view.cpp
- test_ui.cpp
- test_text.cpp
- test_settings.cpp
- app_adapters.cpp
- storage_books.cpp
- tasks.cpp
- test_reader.cpp
- Screen
- RSVP reading on hardware
- Leeloo Device Menu Design
- On-device menu (joystick)
- test_rtos.cpp
- rsvp_service.cpp
- Wi-Fi soft AP and book upload
- App
- Command
- ReadingFrame
- Message
- ends_first_sentence
- view_model.cpp
- test_app.cpp
- InputService
- Event
- Library
- InputFixture
- UiService
- ViewModel
- RsvpFixture
- UiFixture
- RsvpService
- ILibrary
- FakeNotifier
- RsvpMsg
- NetMsg
- test_inactivity.cpp
- UiService::UiService
- src/main.cpp
- Log
- LiPo + TP4056 charging (bring-up, ESP32 NOT connected)
- Snapshot
- Button
- IQueue
- .frame_with
- StorageLibrary
- IMutex
- rtos_freertos.cpp
- INMP441 microphone (bring-up)
- DisplayControl
- .handle_level
- ViewFixture
- IPins
- Enclosure fit (front half)
- RawEdge
- FakeMutex
- State
- BookBuffer
- NvsSettings

## God Nodes (most connected - your core abstractions)
1. `Ui` - 50 edges
2. `press()` - 50 edges
3. `make_ui()` - 49 edges
4. `App` - 42 edges
5. `Reader` - 34 edges
6. `open_root_item()` - 33 edges
7. `Event` - 32 edges
8. `InputFixture` - 29 edges
9. `validate()` - 28 edges
10. `RsvpService` - 27 edges

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

## Communities (85 total, 10 thin omitted)

### Community 0 - "build_enclosure.py"
Cohesion: 0.09
Nodes (31): apply_colours(), box(), build(), build_cover(), chamfer(), check(), circles_sketch(), combine() (+23 more)

### Community 1 - "Leeloo Roadmap"
Cohesion: 0.06
Nodes (76): Agent Context (CONTEXT.md), Hardware purchase gated by board/display/power validation, Project State, Agent Errors and Lessons, Agent Memory (MEMORY.md), Append-only memory rule, Phase 0 scaffold (memory entry), Agent Workflow (WORKFLOW.md) (+68 more)

### Community 2 - "Menu"
Cohesion: 0.09
Nodes (7): utf8_sequence_length(), Menu, count_, test_down_wraps_from_last_to_first(), test_empty_menu_ignores_input(), test_select_returns_current_index(), test_up_wraps_from_first_to_last()

### Community 3 - "hold_of"
Cohesion: 0.12
Nodes (16): hold_of(), test_hold_adds_bonus_for_long_words_counting_codepoints_not_bytes(), test_hold_handles_digits_and_decimals(), test_hold_handles_punctuation_only_token(), test_hold_ignores_carriage_returns_in_paragraph_gap(), test_hold_ignores_opening_marks_at_word_start(), test_hold_is_comma_level_for_closing_quote_paren_and_dash(), test_hold_is_comma_level_for_comma_semicolon_colon() (+8 more)

### Community 4 - "test_buttons.cpp"
Cohesion: 0.09
Nodes (21): classify(), Debouncer, changed_at_, raw_high_, stable_high_, Edge, None, Pressed (+13 more)

### Community 6 - "word_at"
Cohesion: 0.40
Nodes (3): test_next_word_skips_leading_spaces(), test_next_word_treats_tabs_and_newlines_as_separators(), word_at()

### Community 7 - "Temporary input: analog joystick (replaces the 3 buttons)"
Cohesion: 0.40
Nodes (4): Behavior, Checks, Temporary input: analog joystick (replaces the 3 buttons), Wiring

### Community 8 - "rsvp.cpp"
Cohesion: 0.09
Nodes (39): BestSplit, byte, cost, letters, classify(), count_letters(), count_newlines_after(), decode_at() (+31 more)

### Community 9 - "NetService"
Cohesion: 0.06
Nodes (20): NetService, applied_wifi_, auto_session_, kInactivityPollMs, kStopAfterSaveMs, log_, net_, NetService::NetService() (+12 more)

### Community 10 - "OLED RSVP smoke test"
Cohesion: 0.40
Nodes (4): Checks, OLED RSVP smoke test, Run log, Setup

### Community 11 - "Temporary input: 3 push buttons (replaces the EC11 encoder)"
Cohesion: 0.22
Nodes (8): Checks, Firmware notes (when implemented), Full pin summary, Mapping, Menu test (firmware on `dev`), OLED 1.3" 128x64 (I2C, 4 pins, unchanged), Temporary input: 3 push buttons (replaces the EC11 encoder), Wiring (all connections)

### Community 12 - "test_joystick.cpp"
Cohesion: 0.13
Nodes (15): Axis, center_, state_, Direction, High, Low, None, test_hysteresis_does_not_retrigger_between_exit_and_enter() (+7 more)

### Community 13 - "app.js"
Cohesion: 0.08
Nodes (27): vitest, busy(), loadPdfjs(), show(), steps, ApiError, authenticate(), errorFromResponse() (+19 more)

### Community 14 - "upload_server.cpp"
Cohesion: 0.11
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
Cohesion: 0.11
Nodes (33): check_capacity(), test_accepts_book_exactly_at_size_limit(), test_accepts_book_with_pages(), test_accepts_header_line_at_limit(), test_accepts_minimal_valid_book(), test_accepts_spanish_multibyte_content(), test_bytewise_feed_matches_single_chunk(), test_capacity_accepts_last_free_slot() (+25 more)

### Community 19 - "Result"
Cohesion: 0.15
Nodes (14): error_code(), http_status(), Result, BadName, InvalidFormat, InvalidUtf8, LibraryFull, NoSpace (+6 more)

### Community 20 - "percent_decode"
Cohesion: 0.15
Nodes (12): extract_pin(), hex_value(), percent_decode(), pin_matches(), test_extract_pin_reads_the_web_app_body(), test_extract_pin_rejects_malformed_bodies(), test_extract_pin_tolerates_whitespace(), test_percent_decode_decodes_utf8_escapes() (+4 more)

### Community 21 - "upload.cpp"
Cohesion: 0.13
Nodes (16): BookValidator, body_bytes_, error_, line_, line_len_, pages_seen_, state_, total_ (+8 more)

### Community 22 - ".take"
Cohesion: 0.15
Nodes (17): test_book_open_failure_from_rsvp_shows_a_message(), test_book_without_start_marker_fails_and_is_released(), test_cancelled_delete_keeps_the_book(), test_changing_contrast_saves_applies_and_tells_net(), test_changing_wifi_mode_sends_the_new_mode_to_net(), test_choosing_a_book_sends_open_with_its_index(), test_confirmed_delete_removes_the_book_and_resyncs(), test_failed_delete_shows_the_error_and_keeps_the_count() (+9 more)

### Community 23 - "sanitize_name"
Cohesion: 0.12
Nodes (15): is_forbidden_name_byte(), sanitize_name(), Utf8Checker, hi_, lo_, need_, test_sanitize_accepts_name_at_byte_limit(), test_sanitize_accepts_spanish_name() (+7 more)

### Community 25 - "Ui"
Cohesion: 0.08
Nodes (41): Outcome, command, index, Ui, return_to_, book_label(), draw_about(), draw_brightness() (+33 more)

### Community 26 - "rsvp_view.cpp"
Cohesion: 0.16
Nodes (22): clip(), collect(), copy_slice(), draw_at(), draw_layout(), draw_piece(), fits(), Layout (+14 more)

### Community 27 - "test_ui.cpp"
Cohesion: 0.13
Nodes (52): settings_, make_ui(), open_delete_list(), open_root_item(), open_setting(), press(), test_about_is_read_only(), test_auto_brightness_reports_missing_sensor() (+44 more)

### Community 28 - "test_text.cpp"
Cohesion: 0.12
Nodes (22): count_chars(), is_continuation(), offset_after_chars(), scroll_start(), truncate_chars(), valid_utf8(), test_counts_spanish_letters_as_one_character(), test_empty_text_stays_empty() (+14 more)

### Community 29 - "test_settings.cpp"
Cohesion: 0.07
Nodes (32): BrightnessMode, Auto, Manual, cycle_brightness(), cycle_wifi(), display_register(), sanitize(), Settings (+24 more)

### Community 30 - "app_adapters.cpp"
Cohesion: 0.22
Nodes (3): SerialLog, SoftAp, pin_

### Community 31 - "storage_books.cpp"
Cohesion: 0.14
Nodes (20): base_name(), book_count(), book_path_for(), BookText, data, size, find_book(), free_bytes() (+12 more)

### Community 32 - "tasks.cpp"
Cohesion: 0.14
Nodes (13): log, capped(), display_task(), feed(), halt(), input_task(), net_task(), on_book_saved() (+5 more)

### Community 33 - "test_reader.cpp"
Cohesion: 0.06
Nodes (54): body_offset(), is_continuation(), is_space(), previous_word_start(), Reader, beat_ms_, current_, due_ms_ (+46 more)

### Community 34 - "Screen"
Cohesion: 0.13
Nodes (15): Screen, About, Brightness, Contrast, DeleteConfirm, DeleteList, Manage, Message (+7 more)

### Community 35 - "RSVP reading on hardware"
Cohesion: 0.25
Nodes (7): Before flashing, Book opening, Pause and sentence navigation, Playback, Resume and end, RSVP reading on hardware, Storage

### Community 36 - "Leeloo Device Menu Design"
Cohesion: 0.18
Nodes (11): 1. Goals and constraints, 2. Menu hierarchy, 3. Navigation and feedback, 4. OLED layout, 5. Open hardware checks, 6. Implementation decisions (first firmware pass), Ajustes, Biblioteca (+3 more)

### Community 37 - "On-device menu (joystick)"
Cohesion: 0.18
Nodes (10): Ajustes (persistence), Before flashing, Biblioteca and Wi-Fi, Boot, Info, Known limits to watch, Leer and Borrar, Measurements (+2 more)

### Community 38 - "test_rtos.cpp"
Cohesion: 0.09
Nodes (13): FakeClock, now_, FakeQueue, capacity_, item_size_, items_, Message, at_ms (+5 more)

### Community 40 - "Wi-Fi soft AP and book upload"
Cohesion: 0.25
Nodes (7): Before you start (blockers found while writing this), Conversion in the phone, Measurements to record, PIN and session, Session start, Upload and storage, Wi-Fi soft AP and book upload

### Community 41 - "App"
Cohesion: 0.09
Nodes (21): App, ap, contrast, display_wake, edge_port, edges, input, library (+13 more)

### Community 42 - "Command"
Cohesion: 0.18
Nodes (11): Command, CloseBook, DeleteBook, None, OpenBook, ReadingDown, ReadingSelect, ReadingUp (+3 more)

### Community 44 - "ReadingFrame"
Cohesion: 0.20
Nodes (7): ReadingFrame, active, piece, show_wpm, state, word, wpm

### Community 45 - "Message"
Cohesion: 0.25
Nodes (8): Message, ApStartFailed, BookOpenFailed, DeleteFailed, None, UploadSaved, UploadTimedOut, WifiOff

### Community 46 - "ends_first_sentence"
Cohesion: 0.25
Nodes (8): ends_first_sentence(), test_ends_sentence_after_closing_quote_and_period(), test_ends_sentence_for_ellipsis(), test_ends_sentence_for_paragraph_break_without_period(), test_ends_sentence_for_period(), test_ends_sentence_for_question_and_exclamation(), test_ends_sentence_is_false_for_comma(), test_ends_sentence_is_false_for_plain_word()

### Community 47 - "view_model.cpp"
Cohesion: 0.27
Nodes (4): NetStatus, pin, running, update()

### Community 48 - "test_app.cpp"
Cohesion: 0.12
Nodes (21): test_boot_failure_with_wifi_on_is_logged(), test_boot_with_automatico_keeps_the_ap_off(), test_boot_with_wifi_on_starts_the_ap(), test_bounce_that_returns_to_idle_posts_nothing(), test_deferred_stop_sets_the_wait_to_the_remaining_time(), test_ignores_edge_with_button_index_out_of_range(), test_inactivity_check_survives_a_32_bit_clock_wrap(), test_inactivity_ends_an_automatico_session_and_tells_the_ui() (+13 more)

### Community 49 - "InputService"
Cohesion: 0.18
Nodes (9): ButtonState, debouncer, pressed_at, InputService, log_, pins_, settling_, states_ (+1 more)

### Community 50 - "Event"
Cohesion: 0.10
Nodes (19): Event, Back, BookOpenFailed, DeleteFailed, Down, Left, ReaderFinished, Right (+11 more)

### Community 51 - "Library"
Cohesion: 0.13
Nodes (13): Library, closes, count, has_saved, open_ok, opens, remove_ok, removed (+5 more)

### Community 52 - "InputFixture"
Cohesion: 0.14
Nodes (21): book(), InputFixture, log, pins, svc, ui, test_book_with_empty_body_finishes_at_once(), test_close_saves_the_position_and_releases_the_book() (+13 more)

### Community 53 - "UiService"
Cohesion: 0.16
Nodes (10): ILog, post(), UiService, display_, library_, log_, net_out_, rsvp_out_ (+2 more)

### Community 54 - "ViewModel"
Cohesion: 0.25
Nodes (7): ViewModel, display_, mutex_, state_, version_, DisplayContrast, view_

### Community 55 - "RsvpFixture"
Cohesion: 0.17
Nodes (8): Chan, port, q, RsvpFixture, lib, log, svc, ui

### Community 56 - "UiFixture"
Cohesion: 0.14
Nodes (12): SettingsStore, last, ok, saves, UiFixture, display, lib, log (+4 more)

### Community 57 - "RsvpService"
Cohesion: 0.17
Nodes (9): RsvpService, kWpmLabelMs, label_until_ms_, library_, log_, pieces_, reader_, title_ (+1 more)

### Community 59 - "FakeNotifier"
Cohesion: 0.22
Nodes (6): ViewModel::ViewModel(), FakeNotifier, count, depth_at_last, mutex, INotifier

### Community 60 - "RsvpMsg"
Cohesion: 0.20
Nodes (9): RsvpCmd, Close, Down, Open, Select, Up, RsvpMsg, cmd (+1 more)

### Community 61 - "NetMsg"
Cohesion: 0.20
Nodes (10): NetCmd, ApplyWifi, Boot, SavedOnUploadScreen, StartUpload, StopUpload, NetMsg, cmd (+2 more)

### Community 62 - "test_inactivity.cpp"
Cohesion: 0.17
Nodes (10): InactivityTimer, last_ms_, limit_ms_, running_, test_activity_restarts_the_window(), test_does_not_expire_before_the_limit(), test_expires_exactly_at_the_limit(), test_stop_disarms_the_timer() (+2 more)

### Community 63 - "UiService::UiService"
Cohesion: 0.22
Nodes (3): IDisplayControl, ISettingsStore, UiService::UiService()

### Community 65 - "Log"
Cohesion: 0.25
Nodes (5): Log, lines, test_logs_the_drop_when_the_ui_queue_is_full(), test_save_failure_is_logged_not_ignored(), test_settings_save_failure_is_logged_and_still_applied()

### Community 66 - "LiPo + TP4056 charging (bring-up, ESP32 NOT connected)"
Cohesion: 0.33
Nodes (5): Checklist, LiPo + TP4056 charging (bring-up, ESP32 NOT connected), Next, Procedure, Result: battery direct to the devkit 5V pin does NOT work

### Community 67 - "Snapshot"
Cohesion: 0.22
Nodes (6): Snapshot, contrast, contrast_pending, frame, net, ui

### Community 68 - "Button"
Cohesion: 0.50
Nodes (4): Button, kDown, kSelect, kUp

### Community 69 - "IQueue"
Cohesion: 0.29
Nodes (3): IQueue, Queue, port_

### Community 70 - ".frame_with"
Cohesion: 0.33
Nodes (7): test_contrast_request_is_handed_over_once(), test_each_publish_wakes_the_display_outside_the_lock(), test_latest_publish_wins_when_the_display_is_slow(), test_producers_do_not_overwrite_each_others_part(), test_published_state_is_delivered_once(), test_read_balances_the_lock(), test_view_has_nothing_to_read_before_the_first_publish()

### Community 72 - "IMutex"
Cohesion: 0.43
Nodes (3): IMutex, ScopedLock, mutex_

### Community 73 - "rtos_freertos.cpp"
Cohesion: 0.10
Nodes (10): IClock, Clock, Mutex, handle_, Queue, handle_, item_size_, TaskNotifier (+2 more)

### Community 74 - "INMP441 microphone (bring-up)"
Cohesion: 0.50
Nodes (3): Checklist, INMP441 microphone (bring-up), Wiring

### Community 75 - "DisplayControl"
Cohesion: 0.29
Nodes (5): DisplayControl, calls, last, Pins, level

### Community 77 - "ViewFixture"
Cohesion: 0.29
Nodes (6): ViewFixture, display, mutex, seen, snap, view

### Community 79 - "Enclosure fit (front half)"
Cohesion: 0.40
Nodes (4): Checks, Enclosure fit (front + back half), Notes, Setup

### Community 80 - "RawEdge"
Cohesion: 0.33
Nodes (4): RawEdge, button, high, ms

### Community 81 - "FakeMutex"
Cohesion: 0.40
Nodes (3): FakeMutex, depth, locks

### Community 82 - "State"
Cohesion: 0.40
Nodes (5): State, Author, Body, PagesOrStart, Title

### Community 83 - "BookBuffer"
Cohesion: 0.50
Nodes (3): BookBuffer, data, size

## Knowledge Gaps
- **299 isolated node(s):** `debouncer`, `pressed_at`, `pins_`, `ui_out_`, `log_` (+294 more)
  These have ≤1 connection - possible missing edges or undocumented components. (Counts symbols only; 665 node(s) total have ≤1 connection when file, concept and rationale nodes are included.)
- **10 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `App` connect `App` to `tasks.cpp`, `StorageLibrary`, `NetService`, `rtos_freertos.cpp`, `IPins`, `RawEdge`, `InputService`, `Event`, `NvsSettings`, `UiService`, `ViewModel`, `RsvpService`, `RsvpMsg`, `NetMsg`, `app_adapters.cpp`?**
  _High betweenness centrality (0.069) - this node is a cross-community bridge._
- **What connects `debouncer`, `pressed_at`, `pins_` to the rest of the system?**
  _299 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `build_enclosure.py` be split into smaller, more focused modules?**
  _Cohesion score 0.0851063829787234 - nodes in this community are weakly interconnected._
- **Why does `Event` connect `Event` to `NetService`, `App`, `cstdint`, `.handle_level`, `InputService`, `InputFixture`, `UiService`, `.take`, `RsvpFixture`, `RsvpService`, `ILibrary`, `test_ui.cpp`, `Ui`?**
  _High betweenness centrality (0.044) - this node is a cross-community bridge._
- **Should `Leeloo Roadmap` be split into smaller, more focused modules?**
  _Cohesion score 0.06134371957156767 - nodes in this community are weakly interconnected._
- **Why does `RsvpService` connect `RsvpService` to `test_reader.cpp`, `rsvp_service.cpp`, `App`, `cstdint`, `ReadingFrame`, `Event`, `UiService`, `RsvpFixture`, `ILibrary`?**
  _High betweenness centrality (0.035) - this node is a cross-community bridge._
- **Should `Menu` be split into smaller, more focused modules?**
  _Cohesion score 0.08870967741935484 - nodes in this community are weakly interconnected._