# Leeloo Device Menu Design

Status: draft. This document specifies the on-device OLED menu only. The phone web application and its visual design are out of scope.

## 1. Goals and constraints

- Let the user choose a stored book (a `.txt` file), upload a book, delete a book, adjust display settings, and inspect device information.
- Keep all user-facing labels and help text in Spanish.
- Use the 128x64 monochrome OLED and three-button input.
- Keep screens sparse, readable, and usable without color. Follow the OLED typography, inversion, scrolling, and truncation rules in [`DESIGN.md`](DESIGN.md).
- Do not add a web interface, PDF conversion, or enclosure design to this menu.

## 2. Menu hierarchy

Items appear in this order; the focused item is inverted. The five root items
scroll through the four visible OLED rows so focus remains visible.

```text
LEELOO
  Leer >
  Biblioteca >
  Grabar >
  Ajustes >
  Info >
```

Each screen's title is the name of the row that opens it, in capitals (`LEER`,
`BIBLIOTECA`, `GRABAR`, `AJUSTES`, `INFO`). User-facing text says "libro", not
"TXT".

### Leer

Selecting **Leer** opens the list of stored books; this list is the book choice step. Selecting a book starts RSVP at its saved byte position, if it has unfinished progress, or at the first word otherwise. A completed book starts from the beginning when selected again.

- Show book titles in storage order, with long titles truncated to one line.
- Keep a selectable `< Atrás` row fixed on the bottom row; show and scroll books in the three rows above it.
- If there are no books, show `Sin libros`, `Sube uno desde Biblioteca.` (split over two lines), and the selectable `< Atrás` row.
- On return from reading, preserve the current UTF-8-safe byte position as specified in [`ARCHITECTURE.md`](ARCHITECTURE.md).
- At end of book, offer `Releer` and `Volver al menú`.

### Biblioteca

```text
BIBLIOTECA
  Subir >
  Borrar >
  < Atrás
```

Selecting `Atrás` returns to the root menu. The upload screen also shows an `Atrás` action; selecting it cancels the screen/session using the same behavior as a long SELECT press.

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
The book list scrolls in the rows above a fixed, selectable `< Atrás` row, returning to Biblioteca. `Cancelar` remains the back action on the confirmation screen.

### Grabar

The device records mono 16-bit PCM WAV from the I2S microphone wired to BCLK
GPIO15, WS GPIO16 and data GPIO17.

- If no WAV is pending, show `Grabar`. Capture at 8 kHz for up to two minutes
  when LittleFS has enough room while preserving its 2 MiB reserve. If not,
  fall back to 16 kHz for up to 30 seconds. If neither format fits, show that
  the user must free storage; never delete books automatically.
- During capture, show elapsed active recording time and offer `Pausar` and
  `Detener`. While paused, offer `Reanudar` and `Detener`. Paused time does not
  count toward the capture limit. A long SELECT press also stops and finalizes
  the recording.
- After finalization, keep the WAV pending on the device. Only one recording
  can be pending: the user must transfer or delete it before recording again.
- With a pending WAV, show `Subir audio` and `Borrar audio`. Deletion requires
  an explicit confirmation and failure must leave the file intact.
- `Subir audio` starts the existing authenticated soft-AP session and shows
  SSID, PIN and address. The phone page receives and plays the WAV and offers a
  download. The device deletes its copy only after the browser receives the
  complete file and confirms receipt. An interrupted transfer leaves the WAV
  available for retry.
- `Wi-Fi: Apagado` blocks the transfer and directs the user to Ajustes. Auto
  and Encendido keep their existing lifecycle behavior.

Audio transcription is not part of this firmware pass. A future transcript is
separate data and must not be deleted when transferred.

### Ajustes

```text
AJUSTES
  Brillo >
  Contraste >
  Wi-Fi >
  Velocidad lectura >
  < Atrás
```

Keep `< Atrás` fixed on the bottom row; the four settings scroll in the rows above it. The Brillo, Contraste, Wi-Fi, and Velocidad de lectura screens each include a selectable `< Atrás` row to return to Ajustes.

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

Show the selected value as `< Automático >`. SELECT starts editing; UP/DOWN change the value, and SELECT ends editing. Generate a different PIN for each AP session. Deep sleep always stops Wi-Fi; after wake, apply the saved mode again.

**Velocidad de lectura** selects the speed increment used by UP/DOWN during playback: 5, 10, 25, 50, or 100 PPM, with 25 PPM as the default. The value is labeled `Paso` on screen. SELECT starts editing; UP/DOWN choose an increment and SELECT saves/ends editing. The reading speed remains bounded to 100–500 PPM.

The device remembers one global last-used reading speed for all books. A new device starts at 300 PPM; after that, each book starts at the speed last used. Changes are saved when leaving a book or when it finishes, not when pausing. The saved speed is aligned to the selected increment.

### Info

Show a read-only `INFO` screen with:

- Firmware version.
- Battery estimate and charging state, when available.
- Free storage space.
- Wi-Fi mode.

Include a selectable `Atrás` row to return to the root menu. If future values do not fit above it, add button-based paging rather than reducing text below the minimum size in [`DESIGN.md`](DESIGN.md). Identify the board only after its exact model has been verified. Do not display session PINs or other credentials here.

## 3. Navigation and feedback

- UP/DOWN move the focus or change a focused setting. Menu lists wrap at both ends; holding them does not repeat menu navigation.
- During active reading and while editing numeric settings, UP/DOWN act immediately and repeat after 400 ms, then every 150 ms at a constant rate. Wi-Fi/brightness-mode choices do not repeat. While paused, each press seeks one sentence and holding does not repeat.
- Short press selects a menu item, opens a submenu, or confirms a focused action. On navigable secondary screens, the selectable `< Atrás` row returns to that screen's parent.
- Long press returns to the parent screen. From the root menu it enters deep sleep; from Wi-Fi Upload it cancels the session as described above.
- Keep focus visible through inversion. Do not rely on icons or color alone.
- Deletion is the only menu action that removes user data; it always requires a second, explicit confirmation.
- Persist brightness mode/level, contrast, Wi-Fi mode, reading-speed increment, last global reading speed, and per-book reading position. Apply display and Wi-Fi setting changes immediately and retain them across restart.
- Use brief Spanish status/error messages and keep a clear return path. Empty libraries, storage-full conditions, upload failures, and unavailable sensors must not look like successful operations.

## 4. OLED layout

- Use the shared 128x64 grid, margins, type, and inverted focus treatment from [`DESIGN.md`](DESIGN.md).
- Keep each menu item on one line; truncate long titles with `…`.
- Keep `< Atrás` fixed on the bottom row of every screen that shows it. Scroll long book lists within the three rows above it so the focused book remains visible. Do not use horizontal scrolling or scroll indicators. Use a short horizontal slide between menu screens: forward navigation enters from the right and moves left; Back enters from the left and moves right. Do not animate entry to or exit from RSVP reading, or its per-word updates.
- Keep live speed adjustment in the RSVP screen; configure its increment in Ajustes > Velocidad lectura.
- The upload screen may use compact, left-aligned lines for SSID, PIN, and address; ensure the PIN is fully visible.

## 5. Open hardware checks

- Verify the OLED controller and the actual contrast command/range before finalizing percentage mapping.
- Confirm that the ambient light sensor is present and reliable before making Auto brightness the factory default.
- Validate the Wi-Fi inactivity timeout and upload completion/cancel behavior on the device.
- Confirm which battery and free-space values can be reported reliably; omit unavailable values rather than showing fabricated data.

## 6. Implementation decisions (first firmware pass)

Choices the spec left open, recorded so the hardware checks can confirm them:

- **Input:** three buttons (UP GPIO4, DOWN GPIO5, SELECT GPIO6, wiring in `docs/hardware-checklists/buttons-input.md`), not an encoder. UP and DOWN move the cursor one row per press. SELECT short opens or confirms; SELECT long goes back. On navigable secondary screens, a `< Atrás` text row is an additional selectable back action. In Brillo, Contraste, Wi-Fi, and Velocidad de lectura, SELECT short starts editing the marked value (shown as `[ v ]`), UP raises it and DOWN lowers it, and the next SELECT short or Back ends the edit. During active reading and numeric edits, UP/DOWN repeat after 400 ms and every 150 ms; no repeat occurs in menus, categorical settings, or paused sentence seeking. There is no deep sleep yet, so SELECT long does nothing on the root menu.
- **Brillo × Contraste:** the SH1106 has one contrast register. The firmware writes `round(255 × brillo% × contraste% / 100²)`. This is an approximation until the real range is measured.
- **Ambient sensor:** none is wired. Auto brightness shows `Sensor no disponible` and uses the manual level.
- **Wi-Fi default:** `Automático`. The spec does not set one.
- **Battery:** not shown. The firmware cannot measure it yet.
- **Reading controls:** while reading, UP and DOWN change PPM by the configured step (5/10/25/50/100, range 100–500), and `N PPM` shows in the top-left for about 2 s after a change; holding repeats at a constant rate. SELECT short pauses; SELECT long returns to the list. While paused, `PAUSA` shows, UP and DOWN jump one sentence per press, and SELECT short resumes. The latest speed is global across books (initially 300 PPM) and is saved on leaving or finishing a book, not on pause. The position is saved on pause and on leaving the book. Checklist: `docs/hardware-checklists/reading.md`.
- **Firmware version:** `0.1.0-dev`, a constant in `src/display_ui.cpp`.
- **Font:** `u8g2_font_6x10_tf`. The ellipsis is written as `...` because the glyph for `…` is unverified in that font. Confirm on the panel.
- **List window:** book lists use three rows above the fixed `< Atrás` row, scrolled by `lib/text` so the cursor stays visible, with wrap-around at both ends and no scroll indicator.
- **Storage:** LittleFS is mounted without formatting. A blank or corrupt partition shows `Almacenamiento no disponible` and the books are untouched. Flash the filesystem image before the first test.
- **Replace and power loss:** a replaced book is moved aside before the new one is renamed in. A power cut between those two renames loses the old book. Known limit, to revisit if it matters on hardware.
- **Automático after a save:** the AP stays up for 1.5 s so the phone receives its `201`, then stops.
