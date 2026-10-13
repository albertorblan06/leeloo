# Leeloo Architecture Questionnaire

This document captures the decisions required before selecting hardware, libraries, firmware architecture, or implementation details. Answer each question directly, or write **choose a sensible default** and record the resulting decision in the architecture document.

## Product and prototype scope

1. Is Leeloo primarily a personal prototype, a daily portable reader, or a product intended for other users?
2. What must the first functional prototype demonstrate?
3. What is explicitly out of scope for the first milestone?
4. Should the project cover only firmware and the phone web app, or also electronics, enclosure, battery/charging, manufacturing, and test strategy?

## ESP32-S3 and development environment

5. Is there a preferred ESP32-S3 module or development board, or should the plan select one?
6. What constraints matter for the board: PSRAM, flash size, USB, battery charging, dimensions, and availability?
7. Is external PSRAM allowed or required for upload buffering, rendering, or text indexing?
8. Should the firmware baseline be ESP-IDF, Arduino-as-component, or another framework?
9. Is Antigravity IDE a fixed toolchain requirement or only the preferred editor?

## Display and visual design

10. What OLED size, resolution, color capability, controller, and refresh-rate target should be assumed?
11. Is the display strictly I2C, or may SPI be considered if I2C bandwidth is insufficient?
12. What text characteristics are required: Latin only, accented characters, non-Latin scripts, variable font sizes, inverse mode, or custom fonts?
13. Which HUD fields are mandatory: page, completion percentage, time, battery, speed, title, or others?
14. Should an equivalent PDF page number be preserved during phone-side conversion?
15. If page numbers are required, should the upload format include explicit page markers or metadata?

## Input and interaction

16. What encoder hardware is intended: mechanical or optical, detent count, push-button wiring, pull-ups, and expected rotation rate?
17. Should rotation direction and click behavior be configurable?
18. What should long-press, double-click, or click while Wi-Fi mode is active do?
19. During reading, should rotation while playing do nothing, adjust speed, or seek?
20. While paused, what seek granularity is desired: word, sentence, percentage, time, or page marker?
21. What should happen on boot, book completion, an empty library, invalid files, and accidental button presses?
22. Must all encoder events be handled by hardware interrupts, including debouncing and event queuing, or may an interrupt feed a dedicated real-time task?

## RSVP semantics

23. What WPM range, default, and adjustment step are required?
24. Should punctuation, long words, numbers, and sentence boundaries receive timing pauses?
25. What is the RSVP tokenization unit: one word, grouped words, or adaptive chunks?
26. Should the focal or optimal recognition letter be highlighted?
27. Should reading position be saved per book across power cycles?
28. When should progress persist, and how should resume, restart, and reset work?
29. What text encodings and line endings must be accepted?
30. Which Markdown constructs should be rendered, stripped, or rejected?
31. How should malformed, empty, or extremely long input be handled?

## Wi-Fi and PDF-to-text workflow

32. Should the ESP32 operate as a Wi-Fi access point, join an existing network, support both, or use another provisioning method?
33. Should Wi-Fi credentials persist across restarts?
34. Is the upload page limited to modern smartphone browsers?
35. Which platforms must be supported: iOS Safari, Android Chrome, desktop browsers, or others?
36. What maximum PDF size and resulting text size should be supported?
37. Should conversion show progress and support cancellation, retry, and multiple books?
38. How should client-side extraction handle columns, headers, footers, hyphenation, and page breaks?
39. Are scanned PDFs requiring OCR supported?
40. What should happen when a PDF has no selectable text or extraction fails?
41. Should the browser upload plain `.txt`, `.md`, or a custom format with title, page markers, and metadata?
42. Should the device support book rename, delete, replace, and duplicate detection?
43. What Wi-Fi security and upload authorization are required: open temporary AP, password-protected AP, device PIN, or per-session token?
44. Should the device stop serving Wi-Fi automatically after inactivity or upload completion?

## Storage and file lifecycle

45. What flash size and LittleFS partition budget should be reserved for firmware, OTA, books, settings, and recovery?
46. Is OTA firmware update required for the first product version?
47. What library limits are acceptable: number of books, filename length, maximum book length, and total storage?
48. What should happen when storage is full?
49. Should format or clear-memory require confirmation?
50. Should formatting preserve settings or erase everything?
51. Are atomic uploads, crash recovery, checksums, versioned metadata, or import/export of reading progress required?
52. What is the recovery behavior after power loss during upload, deletion, or progress persistence?

## Power, time, and physical product

53. What battery type, capacity, charging method, USB connector, battery gauge, low-battery behavior, and runtime target should be supported?
54. Is a real-time clock required, or is approximate network/phone time sufficient?
55. What should the clock show when the device is offline?
56. Should the device deep-sleep or power-gate the display and Wi-Fi?
57. What wake gestures and idle timeouts are required?
58. What physical size, orientation, enclosure, and control-placement constraints apply?
59. Should the display be readable in darkness, daylight, or both?
60. Are accessibility requirements needed for text size, contrast, tactile controls, or feedback?

## Reliability, security, and development

61. Is secure boot, flash encryption, signed OTA, or protection of uploaded books required?
62. Is Leeloo strictly a trusted personal device?
63. What debug interface is allowed during development: USB serial, WebSerial, LEDs, or another method?
64. What should release builds expose for logging and diagnostics?
65. Are watchdog recovery, safe mode, corrupted-filesystem recovery, brownout handling, and last-position preservation mandatory?
66. What testing deliverables are required: unit tests, host-side RSVP tests, browser tests, hardware-in-the-loop tests, CI, documentation, and schematics?
67. What coding, naming, and licensing constraints apply to firmware, browser code, and third-party libraries?

## Decisions to lock before implementation

- Exact ESP32-S3 board, flash, PSRAM, and power architecture.
- Display dimensions, controller, bus, rendering limits, and font strategy.
- Interrupt/event architecture for the rotary encoder and push-button.
- RSVP timing, tokenization, highlighting, seeking, and persistence semantics.
- Browser PDF extraction contract, page-marker representation, and upload protocol.
- LittleFS partitioning, file format, metadata, recovery, and library limits.
- Wi-Fi provisioning, authorization, lifecycle, and privacy behavior.
- Battery, charging, sleep, time source, enclosure, and accessibility requirements.
- Security posture, diagnostics, recovery behavior, and validation scope.
