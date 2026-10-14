# Leeloo Product Architecture

## 1. Product definition

Leeloo is an embedded Spanish-language RSVP reader. The initial product is an academic embedded-systems prototype designed with a path toward daily personal use. The MVP must demonstrate the complete path from a phone-hosted document conversion flow to local text storage and readable RSVP output on the device.

The enclosure is outside this project scope and will be designed separately. This project covers firmware, the browser application, electronics integration assumptions, power behavior, and automated tests.

## 2. System boundary

The system consists of:

1. **Leeloo device:** ESP32-S3, OLED, EC11 rotary encoder with push-button, battery/power circuitry, and internal flash.
2. **Device firmware:** Arduino-ESP32 framework using the underlying FreeRTOS scheduler and tasks.
3. **Phone web application:** Served by the ESP32 while upload mode is active. It converts a user-selected PDF in the browser and transfers one normalized text book to the device.
4. **Local storage:** LittleFS for books, settings, and reading progress.
5. **Development interface:** USB serial connection to a Mac for flashing and diagnostics.

The ESP32 does not receive or parse PDF files. It receives normalized UTF-8 text only.

## 3. Hardware baseline

### 3.1 Controller

- ESP32-S3 DevKitC-1-class board, N16R8 variant.
- Assumed resources: 16 MB flash and 8 MB PSRAM. The exact module marking and memory map must be verified before purchase.
- Wi-Fi is used only during explicit upload mode.
- Bluetooth is not part of the MVP.

### 3.2 Display

- 0.96-inch monochrome 128x64 OLED.
- I2C only.
- The exact controller must be confirmed before implementation; the display adapter must isolate controller-specific initialization.
- Spanish Latin characters must be supported through a bundled bitmap font or a compatible existing font.
- Brightness is configurable in Settings.

The display is information-constrained. Reading mode therefore prioritizes only WPM and battery percentage. Paused mode may display progress, estimated page, current time, and controls across multiple screens or compact layouts.

### 3.3 Input

- EC11 mechanical rotary encoder with integrated push-button.
- Encoder phase A, phase B, and switch are connected to interrupt-capable GPIOs with defined pull-up/pull-down behavior.
- ISRs perform only timestamp capture and event notification/queueing.
- Debouncing, event classification, long-press detection, and UI actions run in a FreeRTOS input task. No polling loop may read the encoder pins.

### 3.4 Power

- 3.7 V 500 mAh protected LiPo, 403048-class.
- TP4056 USB-C 5 V / 1 A charging module with protection.
- Battery percentage is an estimate unless a dedicated fuel gauge is later added.
- Deep sleep is entered by a long press from the main menu.
- A long press during Wi-Fi mode exits Wi-Fi mode; a long press during reading or pause returns to the main menu.
- Wake is triggered by holding the encoder button during deep sleep, subject to the selected GPIO wake capability.

The charging/power path must be electrically validated before connecting the battery to the prototype board.

## 4. Firmware architecture

### 4.1 Framework and scheduling

The firmware uses Arduino-ESP32 while explicitly using FreeRTOS tasks, queues, event groups, timers, and synchronization primitives where needed.

Logical core ownership:

- **Core 0:** Wi-Fi soft access point, HTTP server, upload processing, and network lifecycle.
- **Core 1:** UI state machine, OLED rendering, RSVP timing, text navigation, and local input handling.

Core pinning is an optimization and isolation boundary, not a license to share mutable state without synchronization. Cross-core communication uses bounded queues or protected state objects.

### 4.2 Main firmware modules

- **Boot and recovery:** startup screen, configuration loading, filesystem mount, validation, and error screen.
- **Input task:** consumes ISR-generated encoder events and emits semantic actions.
- **UI state machine:** Boot, Main Menu, Library, Reading, Paused, Settings, Wi-Fi Upload, and Error.
- **Display service:** I2C transport, font rendering, screen invalidation, brightness, and compact layouts.
- **RSVP engine:** strict word tokenization, focal-letter highlighting, WPM timing, punctuation pauses, speed changes, pause/resume, and word seeking.
- **Book service:** metadata parsing, UTF-8 text access, library enumeration, deletion, and progress lookup.
- **Storage service:** LittleFS access, atomic writes, settings, and progress records.
- **Wi-Fi/upload service:** temporary AP, per-session PIN, browser assets, upload validation, and clean shutdown.
- **Power service:** battery estimate, brightness, deep sleep, wake handling, and low-battery policy.
- **Diagnostics:** USB serial logs in development and reduced error reporting in release builds.

### 4.3 Input semantics

- Boot: show the text logo `Leeloo` until the main menu is ready.
- Rotation in menus: move selection.
- Click in menus: select.
- Rotation while RSVP is playing: increase/decrease WPM in 25-WPM steps.
- Click while reading: pause.
- Rotation while paused: seek by word using a configurable step; the initial default is one word per detent.
- Click while paused: resume.
- Long press in Wi-Fi mode: stop Wi-Fi and return to the prior menu.
- Long press while reading or paused: return to the main menu.
- Long press in the main menu: enter deep sleep.
- Empty library, invalid book, and storage-full states show explicit Spanish error/help screens.
- At end of book: offer return to Library/Main Menu or restart the book.

## 5. Book format and browser workflow

### 5.1 Single-file format

Each uploaded book is one UTF-8 `.txt` file using LF line endings:

```text
TITLE: Example title
AUTHOR: Example author
PAGES: 120
###START###
Normalized words separated by single spaces.
```

The header is metadata, not displayed as RSVP content. `PAGES` is optional; when absent, page telemetry is unavailable but completion percentage remains available.

The browser is responsible for:

- PDF selection and client-side extraction.
- Normalizing whitespace into a continuous word stream.
- Standardizing line endings to LF.
- Encoding as UTF-8.
- Producing the metadata header and `###START###` delimiter.
- Showing conversion/upload progress, cancellation, retry, and a clear extraction error.

Scanned PDFs/OCR are out of scope. Modern iOS Safari and Android Chrome are the primary browser targets; current desktop Chromium-based browsers are useful for development.

### 5.2 Wi-Fi session

- The device starts a temporary soft AP only when the user selects Upload Book.
- Each session generates a new PIN.
- The PIN is shown on the device and required by the web page before upload.
- No credentials persist because the device does not join a home network in the MVP.
- Wi-Fi shuts down when the user exits upload mode or the session is explicitly cancelled.

## 6. Storage and persistence

LittleFS stores books and small metadata/settings files. No OTA partition is required for the MVP.

Initial conservative limits to validate during implementation:

- Maximum normalized book: 1 MiB.
- Maximum library: 20 books, subject to actual partition capacity.
- Maximum filename: 48 bytes after UTF-8-safe sanitization.
- Reserve at least 2 MiB for firmware/recovery and retain a measurable free-space threshold.

These are starting limits, not hardware guarantees; the partition table and upload validator must expose them as constants.

Required persistence:

- Settings: WPM, brightness, seek step, configured time, and display preferences.
- Per-book progress: exact byte offset at the current cursor, saved when leaving a book and on controlled shutdown.
- Atomic temporary-file-then-rename upload and metadata writes.
- Deletion must complete through the storage service and report failure rather than silently continuing.
- Format/clear requires confirmation and preserves device settings by default; a separate factory-reset behavior may be added later.

On power loss, incomplete temporary files are ignored/removed during boot. A filesystem mount failure presents recovery guidance and must not overwrite existing data automatically.

## 7. RSVP behavior

- Operational range: 150–800 WPM.
- Default: 300 WPM.
- Adjustment: 25 WPM.
- Token: exactly one word.
- Spanish punctuation and sentence boundaries add configurable pauses.
- Long words and numeric tokens may receive bounded extra dwell time.
- One focal letter is highlighted where the selected font/layout permits.
- The byte offset is the persistence anchor; the engine reconstructs the current token from the normalized text.
- Completion percentage is based on content offset relative to content length.
- Estimated paused page: `(current_content_byte_offset / total_content_bytes) * PAGES`, clamped to the valid page range.

Because UTF-8 characters may occupy multiple bytes, offsets must always be validated at token boundaries and never split a code point.

## 8. Reliability and diagnostics

- Enable watchdog supervision for tasks that can block.
- Handle brownout and low-battery conditions conservatively.
- Validate metadata, UTF-8, delimiter placement, size limits, and free space before committing an upload.
- Use atomic replacement for files and progress records.
- Keep USB serial diagnostics detailed in development; release builds should expose only actionable error codes/messages.
- Unit-test every implemented pure logic component, especially tokenization, timing, metadata parsing, page estimation, offset recovery, and input-event classification.
- No secure boot, flash encryption, signed OTA, or book encryption is required for this trusted prototype.

## 9. Language and documentation

- Device UI text: Spanish.
- Repository documentation, architecture, source code, and test names: English.
- Third-party dependencies should use permissive licenses compatible with the project; the project license is to be selected before public release.

## 10. Explicit non-goals

- PDF parsing on the ESP32.
- OCR/scanned-PDF support.
- Bluetooth features.
- OTA updates.
- Home-network/cloud connectivity.
- Custom enclosure design.
- Production security hardening.
