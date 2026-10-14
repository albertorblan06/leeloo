# Leeloo

Leeloo is a pocket-sized RSVP (Rapid Serial Visual Presentation) reader concept for focused reading from a small embedded device.

## Concept

The device is planned around an ESP32-S3, a 1.3-inch monochrome 128x64 I2C OLED (likely SH1106; controller not yet verified), and a rotary encoder with an integrated push-button. Books are stored locally as lightweight text files. A phone-friendly web interface is intended to convert PDFs in the browser and transfer the resulting text to the device, keeping document processing off the microcontroller.

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

The initial architecture baseline is now documented. The product decisions are recorded in [`docs/ARCHITECTURE-ANSWERS.md`](docs/ARCHITECTURE-ANSWERS.md), the normalized technical architecture is in [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md), and the delivery roadmap is in [`docs/IMPLEMENTATION-PLAN.md`](docs/IMPLEMENTATION-PLAN.md).

The original discovery questionnaire remains available in [`docs/ARCHITECTURE-QUESTIONS.md`](docs/ARCHITECTURE-QUESTIONS.md). Hardware purchases and firmware implementation should follow the validation gates in the implementation plan, especially verification of the exact N16R8 board memory map, OLED controller, and battery/charging circuit.
