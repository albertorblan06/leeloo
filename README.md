# Leeloo

Leeloo is a pocket-sized RSVP (Rapid Serial Visual Presentation) reader concept for focused reading from a small embedded device.

## Concept

The device is planned around an ESP32-S3, a rectangular OLED display, and a rotary encoder with an integrated push-button. Books are stored locally as lightweight text files. A phone-friendly web interface is intended to convert PDFs in the browser and transfer the resulting text to the device, keeping document processing off the microcontroller.

## High-level experience

1. Open the library and choose a stored book.
2. Read through the text using RSVP presentation.
3. Use the encoder to pause and navigate or adjust the reading experience.
4. Upload books and configure the reader through the device's web interface.

## Major product areas

- **Embedded device:** display, rotary input, local storage, power, and enclosure.
- **RSVP reader:** text presentation, pacing, progress, navigation, and reading position.
- **Phone web interface:** browser-side document conversion and device file transfer.
- **Library and settings:** stored books, speed, brightness, memory management, and device preferences.
- **Reliability:** recovery, persistence, diagnostics, and validation across hardware and software.

## Architecture status

The project is in requirements discovery. Exact hardware, protocols, libraries, file formats, power design, and performance targets are intentionally not locked yet.

The decision questionnaire is maintained in [`docs/ARCHITECTURE-QUESTIONS.md`](docs/ARCHITECTURE-QUESTIONS.md). The implementation architecture will be documented after those decisions are answered.
