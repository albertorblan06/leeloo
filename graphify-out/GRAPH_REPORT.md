# Graph Report - leeloo  (2026-11-27)

## Corpus Check
- 130 files · ~82,438 words
- Verdict: corpus is large enough that graph structure adds value.
- Unclassified: 9 file(s) not represented in the graph (top: .stl 4, (none) 2, .csv 1)

## Summary
- 2268 nodes · 4701 edges · 106 communities (95 shown, 11 thin omitted)
- Extraction: 88% EXTRACTED · 12% INFERRED · 0% AMBIGUOUS · INFERRED: 570 edges (avg confidence: 0.85)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `d8dbc19f`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- build_enclosure.py
- Leeloo Roadmap
- unity
- hold_of
- test_buttons.cpp
- word_at
- Temporary input: analog joystick (replaces the 3 buttons)
- rsvp.cpp
- NetService
- OLED RSVP smoke test
- Temporary input: 3 push buttons (replaces the EC11 encoder)
- RecordingService
- package.json
- upload_server.cpp
- storage_books.cpp
- Leeloo Design Guide
- Upload web API contract
- test_upload.cpp
- Result
- percent_decode
- upload.cpp
- .take
- test_recording.cpp
- build.mjs
- Ui
- rsvp_view.cpp
- test_ui.cpp
- test_recording_service.cpp
- test_settings.cpp
- api.test.js
- app_adapters.cpp
- tasks.cpp
- test_reader.cpp
- RsvpFixture
- RSVP reading on hardware
- Leeloo Device Menu Design
- On-device menu (joystick)
- test_text.cpp
- test_inactivity.cpp
- Wi-Fi soft AP and book upload
- App
- Command
- Event
- Message
- ends_first_sentence
- IRecordingStore
- test_app.cpp
- InputService
- RsvpMsg
- Library
- InputFixture
- UiService
- ViewModel
- RsvpFixture
- test_boot_anim.cpp
- RsvpService
- ILibrary
- pdf.js
- Store
- NetMsg
- audio_adapters.cpp
- UiFixture
- settings_store.cpp
- Log
- LiPo + TP4056 charging (bring-up, ESP32 NOT connected)
- Snapshot
- Button
- sanitize_name
- .frame_with
- FakeQueue
- IMutex
- rtos_freertos.cpp
- DEVICE-MENU.md
- Screen
- ViewFixture
- Leeloo Product Architecture
- Enclosure fit (front half)
- Mic
- Fixture
- Net
- Switching off and on (deep sleep)
- State
- groq.js
- INet
- IMic
- Leeloo Architecture Questionnaire
- CI workflow (ci.yml)
- algorithm
- RSVP behavior (150-800 WPM, word tokens, byte-offset anchor)
- format.js
- Browser-side PDF conversion
- power.cpp
- ReadingFrame
- RecordingMsg
- DisplayControl
- format_pin
- FakeMutex
- ISpeedStore
- app.js
- RecordingFrame
- FakeNotifier
- Debouncer

## God Nodes (most connected - your core abstractions)
1. `press()` - 89 edges
2. `make_ui()` - 80 edges
3. `Ui` - 70 edges
4. `Event` - 59 edges
5. `App` - 53 edges
6. `open_root_item()` - 43 edges
7. `InputFixture` - 40 edges
8. `RecordingService` - 39 edges
9. `Store` - 36 edges
10. `Reader` - 35 edges

## Surprising Connections (you probably didn't know these)
- `3.2 `POST /api/books`` --references--> `busy()`  [INFERRED]
  docs/WEB-API.md → web/app.js
- `3.4 `POST /api/recording/ack`` --references--> `busy()`  [INFERRED]
  docs/WEB-API.md → web/app.js
- `9. Open questions` --references--> `localStorage()`  [INFERRED]
  docs/DESIGN.md → web/test/groq.test.js
- `Theme switching (web app)` --references--> `localStorage()`  [INFERRED]
  docs/DESIGN.md → web/test/groq.test.js
- `Pure logic in lib/ behind thin hardware interfaces` --semantically_similar_to--> `Test strategy`  [INFERRED] [semantically similar]
  CLAUDE.md → docs/ROADMAP.md

## Import Cycles
- None detected.

## Hyperedges (group relationships)
- **I2C peripherals sharing one bus** — docs_architecture_oled_display, docs_architecture_ambient_light_sensor, docs_architecture_haptic_feedback, docs_architecture_i2c_bus_owner [EXTRACTED 1.00]
- **Requirements to architecture to plan to roadmap chain** — docs_architecture_answers, docs_architecture, docs_implementation_plan, docs_roadmap [EXTRACTED 1.00]
- **Phone to device book transfer pipeline** — docs_architecture_browser_conversion, docs_architecture_wifi_session, docs_architecture_book_format, docs_architecture_littlefs_storage, docs_architecture_rsvp_behavior [INFERRED 0.85]

## Communities (106 total, 11 thin omitted)

### Community 0 - "build_enclosure.py"
Cohesion: 0.08
Nodes (31): apply_colours(), box(), build(), build_cover(), chamfer(), check(), circles_sketch(), combine() (+23 more)

### Community 1 - "Leeloo Roadmap"
Cohesion: 0.19
Nodes (22): Agent Context (CONTEXT.md), Agent Errors and Lessons, Agent Memory (MEMORY.md), Append-only memory rule, Agent Workflow (WORKFLOW.md), Canary Flag, Commit Policy, Project file architecture table (+14 more)

### Community 2 - "unity"
Cohesion: 0.05
Nodes (22): Axis, center_, state_, Direction, High, Low, None, utf8_sequence_length() (+14 more)

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
Nodes (41): BestSplit, byte, cost, letters, classify(), count_letters(), count_newlines_after(), decode_at() (+33 more)

### Community 9 - "NetService"
Cohesion: 0.15
Nodes (11): NetService, applied_wifi_, auto_session_, kInactivityPollMs, kStopAfterSaveMs, log_, net_, recording_session_ (+3 more)

### Community 10 - "OLED RSVP smoke test"
Cohesion: 0.40
Nodes (4): Checks, OLED RSVP smoke test, Run log, Setup

### Community 11 - "Temporary input: 3 push buttons (replaces the EC11 encoder)"
Cohesion: 0.22
Nodes (8): Checks, Firmware notes (when implemented), Full pin summary, Mapping, Menu test (firmware on `dev`), OLED 1.3" 128x64 (I2C, 4 pins, unchanged), Temporary input: 3 push buttons (replaces the EC11 encoder), Wiring (all connections)

### Community 12 - "RecordingService"
Cohesion: 0.11
Nodes (15): RecordingService, backlog_, format_, kMaxReadsPerPoll, kPausedPollMs, kPollMs, kReadSamples, locked_ (+7 more)

### Community 13 - "package.json"
Cohesion: 0.11
Nodes (17): pdfjs-dist, author, dependencies, pdfjs-dist, description, devDependencies, vitest, keywords (+9 more)

### Community 14 - "upload_server.cpp"
Cohesion: 0.15
Nodes (17): auth_handler(), authorized(), book_name_from_request(), books_handler(), drain_body(), fail_with(), last_activity_ms(), mark_activity() (+9 more)

### Community 15 - "storage_books.cpp"
Cohesion: 0.13
Nodes (20): base_name(), book_count(), book_path_for(), BookText, data, size, find_book(), free_bytes() (+12 more)

### Community 16 - "Leeloo Design Guide"
Cohesion: 0.11
Nodes (19): 10. Source notes, 1. Principles, 2. Web app tokens (measured on Loudop), 3.1 Grid, 3.2 Hierarchy, 3.3 Scrolling and wrapping, 3.4 Motion, 3. OLED rules (128x64, monochrome) (+11 more)

### Community 17 - "Upload web API contract"
Cohesion: 0.15
Nodes (14): 1. Session lifecycle, 2. Static assets, 3.1 `POST /api/auth`, 3.2 `POST /api/books`, 3.3 `GET /api/recording`, 3.4 `POST /api/recording/ack`, 3.5 Not a device endpoint: transcription, 3. Endpoints (+6 more)

### Community 18 - "test_upload.cpp"
Cohesion: 0.09
Nodes (37): check_capacity(), ack(), test_accepts_book_exactly_at_size_limit(), test_accepts_book_with_pages(), test_accepts_header_line_at_limit(), test_accepts_minimal_valid_book(), test_accepts_spanish_multibyte_content(), test_ack_bytes_accepts_nine_digits() (+29 more)

### Community 19 - "Result"
Cohesion: 0.12
Nodes (19): error_code(), http_status(), Result, BadName, Busy, InvalidAck, InvalidFormat, InvalidUtf8 (+11 more)

### Community 20 - "percent_decode"
Cohesion: 0.14
Nodes (13): extract_ack_bytes(), extract_pin(), hex_value(), percent_decode(), pin_matches(), test_extract_pin_reads_the_web_app_body(), test_extract_pin_rejects_malformed_bodies(), test_extract_pin_tolerates_whitespace() (+5 more)

### Community 21 - "upload.cpp"
Cohesion: 0.11
Nodes (18): BookValidator, body_bytes_, error_, line_, line_len_, pages_seen_, state_, total_ (+10 more)

### Community 22 - ".take"
Cohesion: 0.11
Nodes (29): hold(), start_recording(), test_a_late_wake_up_posts_one_repeat_and_skips_the_missed_beats(), test_a_lost_start_command_returns_from_the_recording_screen_with_an_error(), test_a_transfer_on_another_screen_sends_nothing_to_the_net_service(), test_audio_upload_and_delete_commands_reach_their_services(), test_book_open_failure_from_rsvp_shows_a_message(), test_book_without_start_marker_fails_and_is_released() (+21 more)

### Community 23 - "test_recording.cpp"
Cohesion: 0.07
Nodes (35): bytes_to_write(), Format, max_pcm_bytes, max_seconds, sample_rate, make_wav_header(), pause_expired(), pcm_bytes_for() (+27 more)

### Community 25 - "Ui"
Cohesion: 0.07
Nodes (49): Outcome, command, index, Ui, entering_back_, return_to_, book_label(), compose_slide_frame() (+41 more)

### Community 26 - "rsvp_view.cpp"
Cohesion: 0.16
Nodes (22): clip(), collect(), copy_slice(), draw_at(), draw_layout(), draw_piece(), fits(), Layout (+14 more)

### Community 27 - "test_ui.cpp"
Cohesion: 0.09
Nodes (95): settings_, expect_back_row_returns_to(), make_ui(), open_delete_list(), open_record_menu(), open_root_item(), open_setting(), press() (+87 more)

### Community 28 - "test_recording_service.cpp"
Cohesion: 0.11
Nodes (34): free_for(), test_a_backlog_longer_than_one_poll_is_continued_without_waiting(), test_a_different_amount_of_free_space_gives_a_different_limit(), test_a_failed_delete_is_reported_and_keeps_the_file(), test_a_failed_finalize_removes_the_temp_file_and_reports(), test_a_full_disk_mid_recording_saves_what_was_captured(), test_a_long_pause_saves_the_recording_and_frees_the_storage(), test_a_microphone_error_before_any_audio_aborts_and_reports() (+26 more)

### Community 29 - "test_settings.cpp"
Cohesion: 0.07
Nodes (30): adjust_reading_step(), cycle_wifi(), display_register(), sanitize(), Settings, brightness_percent, reading_step_ppm, wifi (+22 more)

### Community 30 - "api.test.js"
Cohesion: 0.18
Nodes (11): acknowledgeRecording(), ApiError, authenticate(), errorFromResponse(), errorMessage(), fetchRecording(), messages, uploadBook() (+3 more)

### Community 31 - "app_adapters.cpp"
Cohesion: 0.12
Nodes (7): NvsSettings, NvsSpeed, SerialLog, SoftAp, pin_, StorageLibrary, text_

### Community 32 - "tasks.cpp"
Cohesion: 0.11
Nodes (15): log, capped(), display_task(), feed(), halt(), input_task(), net_task(), on_book_saved() (+7 more)

### Community 33 - "test_reader.cpp"
Cohesion: 0.05
Nodes (59): body_offset(), is_continuation(), is_space(), previous_word_start(), Reader, beat_ms_, current_, due_ms_ (+51 more)

### Community 34 - "RsvpFixture"
Cohesion: 0.13
Nodes (10): RsvpFixture, lib, log, speed, svc, ui, SpeedStore, ok (+2 more)

### Community 35 - "RSVP reading on hardware"
Cohesion: 0.25
Nodes (7): Before flashing, Book opening, Pause and sentence navigation, Playback, Resume and end, RSVP reading on hardware, Storage

### Community 36 - "Leeloo Device Menu Design"
Cohesion: 0.15
Nodes (13): 1. Goals and constraints, 2. Menu hierarchy, 3. Navigation and feedback, 4. OLED layout, 5. Open hardware checks, 6. Implementation decisions (first firmware pass), Ajustes, Apagar (+5 more)

### Community 37 - "On-device menu (joystick)"
Cohesion: 0.18
Nodes (10): Ajustes (persistence), Before flashing, Biblioteca and Wi-Fi, Boot, Info, Known limits to watch, Leer and Borrar libro, Measurements (+2 more)

### Community 38 - "test_text.cpp"
Cohesion: 0.07
Nodes (34): count_chars(), is_continuation(), offset_after_chars(), scroll_start(), truncate_chars(), valid_utf8(), message_text(), MessageText (+26 more)

### Community 39 - "test_inactivity.cpp"
Cohesion: 0.17
Nodes (10): InactivityTimer, last_ms_, limit_ms_, running_, test_activity_restarts_the_window(), test_does_not_expire_before_the_limit(), test_expires_exactly_at_the_limit(), test_stop_disarms_the_timer() (+2 more)

### Community 40 - "Wi-Fi soft AP and book upload"
Cohesion: 0.25
Nodes (7): Before you start (blockers found while writing this), Conversion in the phone, Measurements to record, PIN and session, Session start, Upload and storage, Wi-Fi soft AP and book upload

### Community 41 - "App"
Cohesion: 0.06
Nodes (28): GpioPins, App, ap, contrast, display_wake, edge_port, edges, input (+20 more)

### Community 42 - "Command"
Cohesion: 0.10
Nodes (20): Command, CloseBook, DeleteBook, DeleteRecording, None, OpenBook, ReadingDown, ReadingDownRepeat (+12 more)

### Community 44 - "Event"
Cohesion: 0.07
Nodes (29): Event, AudioAvailable, AudioBusy, AudioDeleted, AudioDeleteFailed, AudioNoSpace, AudioSaved, AudioSaveFailed (+21 more)

### Community 45 - "Message"
Cohesion: 0.11
Nodes (18): Message, ApStartFailed, AudioBusy, AudioDeleted, AudioDeleteFailed, AudioNoSpace, AudioSaved, AudioSaveFailed (+10 more)

### Community 46 - "ends_first_sentence"
Cohesion: 0.25
Nodes (8): ends_first_sentence(), test_ends_sentence_after_closing_quote_and_period(), test_ends_sentence_for_ellipsis(), test_ends_sentence_for_paragraph_break_without_period(), test_ends_sentence_for_period(), test_ends_sentence_for_question_and_exclamation(), test_ends_sentence_is_false_for_comma(), test_ends_sentence_is_false_for_plain_word()

### Community 48 - "test_app.cpp"
Cohesion: 0.09
Nodes (32): test_a_book_session_after_a_recording_session_times_out_as_a_book_session(), test_a_confirmed_transfer_keeps_the_ap_up_in_on_mode(), test_a_confirmed_transfer_stops_the_ap_after_a_short_grace_in_auto_mode(), test_a_lost_release_edge_does_not_repeat_forever(), test_boot_failure_with_wifi_on_is_logged(), test_boot_with_automatico_keeps_the_ap_off(), test_boot_with_wifi_on_starts_the_ap(), test_bounce_that_returns_to_idle_posts_nothing() (+24 more)

### Community 49 - "InputService"
Cohesion: 0.16
Nodes (10): InputService, InputService::InputService(), log_, pins_, settling_, states_, ui_out_, repeats() (+2 more)

### Community 50 - "RsvpMsg"
Cohesion: 0.11
Nodes (16): RawEdge, button, high, ms, RsvpCmd, Close, Down, DownRepeat (+8 more)

### Community 51 - "Library"
Cohesion: 0.10
Nodes (16): BookBuffer, data, size, Library, closes, count, has_saved, open_ok (+8 more)

### Community 52 - "InputFixture"
Cohesion: 0.11
Nodes (27): book(), InputFixture, log, pins, svc, ui, test_book_with_empty_body_finishes_at_once(), test_close_saves_a_changed_speed_once() (+19 more)

### Community 53 - "UiService"
Cohesion: 0.10
Nodes (14): IDisplayControl, ILog, ISettingsStore, post(), UiService, display_, library_, log_ (+6 more)

### Community 54 - "ViewModel"
Cohesion: 0.16
Nodes (8): update(), ViewModel, display_, mutex_, state_, version_, DisplayContrast, view_

### Community 55 - "RsvpFixture"
Cohesion: 0.18
Nodes (8): Chan, port, q, NetFixture, log, net, svc, ui

### Community 56 - "test_boot_anim.cpp"
Cohesion: 0.07
Nodes (33): beat_permille(), eased_permille(), Frame, frame_at(), done, dot_permille, hand_degrees, letters (+25 more)

### Community 57 - "RsvpService"
Cohesion: 0.14
Nodes (13): RsvpService, kWpmLabelMs, label_until_ms_, last_wpm_, library_, log_, pieces_, reader_ (+5 more)

### Community 59 - "pdf.js"
Cohesion: 0.26
Nodes (6): vitest, normalizeText(), ExtractError, extractErrorMessage(), extractPdf(), messages

### Community 60 - "Store"
Cohesion: 0.07
Nodes (19): Store, aborts, cleanups, data, finish_ok, finish_pcm, finish_rate, finishes (+11 more)

### Community 61 - "NetMsg"
Cohesion: 0.15
Nodes (13): NetCmd, ApplyWifi, Boot, RecordingTransferred, SavedOnUploadScreen, StartRecordingUpload, StartUpload, StopRecordingUpload (+5 more)

### Community 62 - "audio_adapters.cpp"
Cohesion: 0.12
Nodes (5): I2sMic, installed_, LittleFsRecordingStore, file_, locked_

### Community 63 - "UiFixture"
Cohesion: 0.13
Nodes (13): SettingsStore, last, ok, saves, UiFixture, display, lib, log (+5 more)

### Community 64 - "settings_store.cpp"
Cohesion: 0.32
Nodes (4): load(), load_reading_wpm(), save(), save_reading_wpm()

### Community 65 - "Log"
Cohesion: 0.22
Nodes (7): Log, lines, test_a_failed_speed_save_is_logged_and_retried_on_the_next_exit(), test_full_net_queue_is_logged_instead_of_dropped_silently(), test_logs_the_drop_when_the_ui_queue_is_full(), test_save_failure_is_logged_not_ignored(), test_settings_save_failure_is_logged_and_still_applied()

### Community 66 - "LiPo + TP4056 charging (bring-up, ESP32 NOT connected)"
Cohesion: 0.22
Nodes (8): Cell change (2026-11-26), Checklist, Incident: fire on the TP4056 (2026-11-26), LiPo + TP4056 charging (bring-up, ESP32 NOT connected), Next, Procedure, Result: battery direct to the devkit 5V pin does NOT work, Working power path (2026-11-27)

### Community 67 - "Snapshot"
Cohesion: 0.18
Nodes (10): NetStatus, pin, running, Snapshot, contrast, contrast_pending, frame, net (+2 more)

### Community 68 - "Button"
Cohesion: 0.50
Nodes (4): Button, kDown, kSelect, kUp

### Community 69 - "sanitize_name"
Cohesion: 0.20
Nodes (10): sanitize_name(), test_sanitize_accepts_name_at_byte_limit(), test_sanitize_accepts_spanish_name(), test_sanitize_counts_bytes_not_characters(), test_sanitize_rejects_empty_and_blank(), test_sanitize_rejects_invalid_utf8(), test_sanitize_rejects_name_over_byte_limit(), test_sanitize_rejects_path_separators_and_controls() (+2 more)

### Community 70 - ".frame_with"
Cohesion: 0.31
Nodes (8): test_contrast_request_is_handed_over_once(), test_each_publish_wakes_the_display_outside_the_lock(), test_latest_publish_wins_when_the_display_is_slow(), test_producers_do_not_overwrite_each_others_part(), test_published_state_is_delivered_once(), test_read_balances_the_lock(), test_the_recording_frame_is_delivered_and_does_not_touch_the_reading_frame(), test_view_has_nothing_to_read_before_the_first_publish()

### Community 71 - "FakeQueue"
Cohesion: 0.07
Nodes (16): FakeClock, now_, FakeQueue, capacity_, item_size_, items_, IQueue, Queue (+8 more)

### Community 72 - "IMutex"
Cohesion: 0.43
Nodes (3): IMutex, ScopedLock, mutex_

### Community 73 - "rtos_freertos.cpp"
Cohesion: 0.10
Nodes (10): IClock, Clock, Mutex, handle_, Queue, handle_, item_size_, TaskNotifier (+2 more)

### Community 74 - "DEVICE-MENU.md"
Cohesion: 0.17
Nodes (10): Before, Capture, Delete, Power, Transcription and summary (phone page), Transfer, Voice recording (Grabadora), Checklist (+2 more)

### Community 76 - "Screen"
Cohesion: 0.10
Nodes (21): Screen, About, Brightness, DeleteConfirm, DeleteList, Manage, Message, PowerConfirm (+13 more)

### Community 77 - "ViewFixture"
Cohesion: 0.29
Nodes (6): ViewFixture, display, mutex, seen, snap, view

### Community 78 - "Leeloo Product Architecture"
Cohesion: 0.23
Nodes (19): Hardware purchase gated by board/display/power validation, Project State, Ambient light sensor, Architecture Answers (requirements), Addendum 2026-10-20 (light sensor and haptics), EC11 rotary encoder input, ESP32-S3 N16R8 controller, Haptic feedback (+11 more)

### Community 79 - "Enclosure fit (front half)"
Cohesion: 0.40
Nodes (4): Checks, Enclosure fit (front + back half), Notes, Setup

### Community 80 - "Mic"
Cohesion: 0.11
Nodes (14): Mic, chunk_samples, pause_ok, pauses, rate, read_fails, reads, reads_left (+6 more)

### Community 81 - "Fixture"
Cohesion: 0.11
Nodes (13): Fixture, log, mic, port, store, svc, ui, Log (+5 more)

### Community 82 - "Net"
Cohesion: 0.17
Nodes (7): Net, is_running, last_activity, marks, start_ok, starts, stops

### Community 83 - "Switching off and on (deep sleep)"
Cohesion: 0.33
Nodes (5): Estimated power budget (2026-11-27, NOT measured), Power, Switching off, Switching off and on (deep sleep), Switching on

### Community 84 - "State"
Cohesion: 0.40
Nodes (5): State, Author, Body, PagesOrStart, Title

### Community 85 - "groq.js"
Cohesion: 0.17
Nodes (17): browserStorage(), codeFromResponse(), forgetKey(), GroqError, groqErrorMessage(), isPlausibleKey(), kGroqBase, kKeyStorageName (+9 more)

### Community 88 - "Leeloo Architecture Questionnaire"
Cohesion: 0.18
Nodes (11): Decisions to lock before implementation, Display and visual design, ESP32-S3 and development environment, Input and interaction, Leeloo Architecture Questionnaire, Power, time, and physical product, Product and prototype scope, Reliability, security, and development (+3 more)

### Community 89 - "CI workflow (ci.yml)"
Cohesion: 0.31
Nodes (10): Phase 0 scaffold (memory entry), Coverage targets (90% firmware, 80% web), Milestone 1: Project and toolchain foundation, CI blocked by GitHub billing issue (TODO), Phase 0 - Repo and test foundation, Phase 5 - Retrofit tests and CI hardening, CI workflow (ci.yml), Build esp32s3 step (compile-only) (+2 more)

### Community 91 - "RSVP behavior (150-800 WPM, word tokens, byte-offset anchor)"
Cohesion: 0.39
Nodes (8): Required edge cases, Single-file UTF-8 book format, LittleFS storage and atomic writes, RSVP behavior (150-800 WPM, word tokens, byte-offset anchor), Milestone 4: Storage and book service, Milestone 5: RSVP engine, Phase 3 - Storage and book service, Phase 4 - RSVP engine

### Community 92 - "format.js"
Cohesion: 0.40
Nodes (8): buildBookFile(), encoder, headerValue(), utf8ByteLength(), kMaxBookBytes, kMaxHeaderValueBytes, kMaxNameBytes, bookNameFrom()

### Community 93 - "Browser-side PDF conversion"
Cohesion: 0.47
Nodes (6): MVP definition (phone uploads PDF, converts to txt, ESP32 renders RSVP), Browser-side PDF conversion, Explicit non-goals, Wi-Fi soft AP session with PIN, Milestone 6: Browser upload application, Phase 6 - Browser upload application

### Community 94 - "power.cpp"
Cohesion: 0.10
Nodes (4): confirm_wake_or_sleep(), enter_deep_sleep(), sleep_now(), size()

### Community 95 - "ReadingFrame"
Cohesion: 0.20
Nodes (7): ReadingFrame, active, piece, show_wpm, state, word, wpm

### Community 96 - "RecordingMsg"
Cohesion: 0.29
Nodes (7): RecordingCmd, Delete, Start, Stop, TogglePause, RecordingMsg, cmd

### Community 97 - "DisplayControl"
Cohesion: 0.29
Nodes (5): DisplayControl, calls, last, Pins, level

### Community 98 - "format_pin"
Cohesion: 0.20
Nodes (7): format_pin(), make_route(), register_handlers(), start(), stop(), test_format_pin_pads_with_zeros(), test_format_pin_wraps_modulo_ten_thousand()

### Community 99 - "FakeMutex"
Cohesion: 0.40
Nodes (3): FakeMutex, depth, locks

### Community 101 - "app.js"
Cohesion: 0.20
Nodes (6): loadPdfjs(), showNotes(), steps, storage, buildNotesText(), ./vendor/pdf.mjs

### Community 104 - "RecordingFrame"
Cohesion: 0.25
Nodes (6): RecordingFrame, active, elapsed_seconds, max_seconds, paused, sample_rate

### Community 109 - "FakeNotifier"
Cohesion: 0.22
Nodes (6): ViewModel::ViewModel(), FakeNotifier, count, depth_at_last, mutex, INotifier

### Community 111 - "Debouncer"
Cohesion: 0.33
Nodes (5): ButtonState, debouncer, next_repeat_at, pressed_at, repeating

## Knowledge Gaps
- **446 isolated node(s):** `debouncer`, `pressed_at`, `next_repeat_at`, `repeating`, `pins_` (+441 more)
  These have ≤1 connection - possible missing edges or undocumented components. (Counts symbols only; 924 node(s) total have ≤1 connection when file, concept and rationale nodes are included.)
- **11 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `Event` connect `Event` to `NetService`, `RecordingService`, `.take`, `Ui`, `test_ui.cpp`, `test_recording_service.cpp`, `RsvpFixture`, `App`, `cstdint`, `InputService`, `InputFixture`, `UiService`, `RsvpFixture`, `RsvpService`, `ILibrary`, `recording_service.cpp`, `Fixture`, `INet`, `IMic`?**
  _High betweenness centrality (0.065) - this node is a cross-community bridge._
- **What connects `debouncer`, `pressed_at`, `next_repeat_at` to the rest of the system?**
  _446 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `build_enclosure.py` be split into smaller, more focused modules?**
  _Cohesion score 0.07764705882352942 - nodes in this community are weakly interconnected._
- **Why does `App` connect `App` to `RecordingMsg`, `tasks.cpp`, `NetService`, `rtos_freertos.cpp`, `RecordingService`, `Event`, `InputService`, `RsvpMsg`, `UiService`, `ViewModel`, `RsvpService`, `NetMsg`, `audio_adapters.cpp`, `app_adapters.cpp`?**
  _High betweenness centrality (0.052) - this node is a cross-community bridge._
- **Should `unity` be split into smaller, more focused modules?**
  _Cohesion score 0.052525252525252523 - nodes in this community are weakly interconnected._
- **Why does `Ui` connect `Ui` to `unity`, `Snapshot`, `cstdint`, `Screen`, `Message`, `UiService`, `ViewModel`, `test_ui.cpp`?**
  _High betweenness centrality (0.039) - this node is a cross-community bridge._
- **Should `hold_of` be split into smaller, more focused modules?**
  _Cohesion score 0.11764705882352941 - nodes in this community are weakly interconnected._