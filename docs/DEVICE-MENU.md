# Leeloo Device Menu Design

Status: draft. This document specifies the on-device OLED menu only. The phone web application and its visual design are out of scope.

## 1. Goals and constraints

- Let the user choose a stored book (a `.txt` file), upload a book, delete a book, adjust display settings, and inspect device information.
- Keep all user-facing labels and help text in Spanish.
- Use the 128x64 monochrome OLED and rotary encoder with push-button.
- Keep screens sparse, readable, and usable without color. Follow the OLED typography, inversion, scrolling, and truncation rules in [`DESIGN.md`](DESIGN.md).
- Do not add a web interface, PDF conversion, reading-speed settings, or enclosure design to this menu.

## 2. Menu hierarchy

Items appear in this order; the focused item is inverted.

```text
LEELOO
  Leer >
  Biblioteca >
  Ajustes >
  Info >
```

Each screen's title is the name of the row that opens it, in capitals (`LEER`, `BIBLIOTECA`, `AJUSTES`, `INFO`). User-facing text says "libro", not "TXT".

### Leer

Selecting **Leer** opens the list of stored books; this list is the book choice step. Selecting a book starts RSVP at its saved byte position, if it has unfinished progress, or at the first word otherwise. A completed book starts from the beginning when selected again.

- Show book titles in storage order, with long titles truncated to one line.
- If there are no books, show `Sin libros` and `Sube uno desde Biblioteca.` (split over two lines)
- On return from reading, preserve the current UTF-8-safe byte position as specified in [`ARCHITECTURE.md`](ARCHITECTURE.md).
- At end of book, offer `Releer` and `Volver al menú`.

### Biblioteca

```text
BIBLIOTECA
  Subir >
  Borrar >
```

**Subir** starts the device's temporary Wi-Fi upload session when Wi-Fi is not configured as Off. The OLED shows the SSID, one-time PIN, and address needed to connect. It may show connection/upload progress, then a short success or error state. This document specifies only what the device displays; the mobile page and conversion workflow are out of scope.

- If Wi-Fi is Off, do not enable it implicitly. Show `Wi-Fi desactivado` and `Actívalo en Ajustes.` with a return action.
- If the AP cannot start, show `No se pudo iniciar Wi-Fi` and allow the user to return.
- In Auto mode, stop the AP after a successful upload, explicit cancellation, or five minutes without upload activity. The timeout is a design default to validate during integration.
- In On mode, keep the AP available while the device is awake. Entering this screen shows the current session details.
- A long press exits the upload screen and stops an Auto-mode session. It does not override the persistent On setting.

**Borrar** opens the book list. After a book is selected, require an explicit confirmation:

```text
¿Borrar este libro?
  Cancelar
  Borrar
```

Focus `Cancelar` by default. On confirmation, delete the file and its saved reading position, then return to the updated list. If the operation fails, keep the file and show an error with a way back.

### Ajustes

```text
AJUSTES
  Brillo >
  Contraste >
  Wi-Fi >
```

**Brillo** contains:

- `Modo`: `Auto` or `Manual`.
- `Nivel manual`: a 10–100% value, adjustable in 5% steps, shown only in Manual mode.
- In Auto mode, use the ambient light sensor. If it is unavailable, use the saved manual level and show `Sensor no disponible` on the brightness screen.

The factory brightness mode/level must be finalized after the sensor and OLED have been validated on hardware. Until then, retain a manual fallback level.

**Contraste** is a separate 10–100% setting, adjustable in 5% steps. Map the percentage to the confirmed OLED driver's supported range; do not expose raw driver values. Use 100% as the provisional default to match the high-contrast OLED design. Confirm the useful range and default on the actual module.

This user-requested control is an explicit exception to [`DESIGN.md`](DESIGN.md)'s general rule to keep OLED contrast at maximum; the design guide itself remains unchanged.

**Wi-Fi** cycles through three persistent choices:

| Label | Device behavior |
|---|---|
| `Apagado` | Keep the access point off. `Subir` asks the user to enable Wi-Fi in Ajustes. |
| `Automático` | Keep Wi-Fi off until `Subir` is selected; stop the upload session on success, cancellation, or inactivity. |
| `Encendido` | Keep the access point available while the device is awake. |

Show the selected value as `< Automático >` and change it with encoder rotation. Generate a different PIN for each AP session. Deep sleep always stops Wi-Fi; after wake, apply the saved mode again.

### Info

Show a read-only `INFO` screen with:

- Firmware version.
- Battery estimate and charging state, when available.
- Free storage space.
- Wi-Fi mode.

Use paging or encoder rotation if all values do not fit; do not reduce text below the minimum size in [`DESIGN.md`](DESIGN.md). Identify the board only after its exact model has been verified. Do not display session PINs or other credentials here.

## 3. Navigation and feedback

- Rotate the encoder to move the focus or change the focused setting. Menu lists wrap at both ends.
- Short press selects a menu item, opens a submenu, or confirms a focused action.
- Long press returns to the parent screen. From the root menu it enters deep sleep; from Wi-Fi Upload it cancels the session as described above.
- Keep focus visible through inversion. Do not rely on icons or color alone.
- Deletion is the only menu action that removes user data; it always requires a second, explicit confirmation.
- Persist brightness mode/level, contrast, Wi-Fi mode, and per-book reading position. Apply display and Wi-Fi setting changes immediately and retain them across restart.
- Use brief Spanish status/error messages and keep a clear return path. Empty libraries, storage-full conditions, upload failures, and unavailable sensors must not look like successful operations.

## 4. OLED layout

- Use the shared 128x64 grid, margins, type, and inverted focus treatment from [`DESIGN.md`](DESIGN.md).
- Keep each menu item on one line; truncate long titles with `…`.
- Scroll long book lists so the focused item remains visible. Do not use horizontal scrolling or animations.
- Keep reading-only controls such as WPM adjustment in the RSVP screen, not in this menu.
- The upload screen may use compact, left-aligned lines for SSID, PIN, and address; ensure the PIN is fully visible.

## 5. Open hardware checks

- Verify the OLED controller and the actual contrast command/range before finalizing percentage mapping.
- Confirm that the ambient light sensor is present and reliable before making Auto brightness the factory default.
- Validate the Wi-Fi inactivity timeout and upload completion/cancel behavior on the device.
- Confirm which battery and free-space values can be reported reliably; omit unavailable values rather than showing fabricated data.

## 6. Implementation decisions (first firmware pass)

Choices the spec left open, recorded so the hardware checks can confirm them:

- **Input:** three buttons (UP GPIO4, DOWN GPIO5, SELECT GPIO6, wiring in `docs/hardware-checklists/buttons-input.md`), not an encoder. UP and DOWN move the cursor. SELECT short opens or confirms; SELECT long goes back. In Brillo, Contraste and Wi-Fi, SELECT short starts editing the marked value (shown as `[ v ]`), UP raises it and DOWN lowers it, and the next SELECT short or Back ends the edit. There is no deep sleep yet, so SELECT long does nothing on the root menu.
- **Brillo × Contraste:** the SH1106 has one contrast register. The firmware writes `round(255 × brillo% × contraste% / 100²)`. This is an approximation until the real range is measured.
- **Ambient sensor:** none is wired. Auto brightness shows `Sensor no disponible` and uses the manual level.
- **Wi-Fi default:** `Automático`. The spec does not set one.
- **Battery:** not shown. The firmware cannot measure it yet.
- **Reading controls:** while reading, UP and DOWN change the WPM by 25 (150–800), and `N WPM` shows in the top-left for about 2 s after a change; SELECT short pauses; SELECT long returns to the list. While paused, `PAUSA` shows, UP and DOWN jump to the previous and next sentence, and SELECT short resumes. The start speed is 300 WPM, not saved between books. The position is saved on pause and on leaving the book, not continuously. Checklist: `docs/hardware-checklists/reading.md`.
- **Firmware version:** `0.1.0-dev`, a constant in `src/display_ui.cpp`.
- **Font:** `u8g2_font_6x10_tf`. The ellipsis is written as `...` because the glyph for `…` is unverified in that font. Confirm on the panel.
- **List window:** four rows, scrolled by `lib/text` so the cursor stays visible, with wrap-around at both ends.
- **Storage:** LittleFS is mounted without formatting. A blank or corrupt partition shows `Almacenamiento no disponible` and the books are untouched. Flash the filesystem image before the first test.
- **Replace and power loss:** a replaced book is moved aside before the new one is renamed in. A power cut between those two renames loses the old book. Known limit, to revisit if it matters on hardware.
- **Automático after a save:** the AP stays up for 1.5 s so the phone receives its `201`, then stops.
