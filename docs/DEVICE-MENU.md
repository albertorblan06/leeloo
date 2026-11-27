# Leeloo Device Menu Design

Status: implemented. This document specifies the on-device OLED menu only. The phone web application and its visual design are out of scope.

## 1. Goals and constraints

- Let the user choose a stored book (a `.txt` file), add a book from the phone, delete a book, record a voice note and send it to the phone, adjust brightness, reading-speed step and Wi-Fi, inspect device information, and switch the device off.
- Keep all user-facing labels and help text in Spanish, with the same word for the same thing everywhere: *libro* for books, *grabación* for recordings, *móvil* for the phone, *dispositivo* for Leeloo.
- Use the 128x64 monochrome OLED and three-button input.
- Keep screens sparse, readable, and usable without color. Follow the OLED typography, inversion, scrolling, and truncation rules in [`DESIGN.md`](DESIGN.md).
- Do not add a web interface, PDF conversion, or enclosure design to this menu.

## 2. Menu hierarchy

Items appear in this order; the focused item is inverted. Every screen below the root ends with a selectable `< Atrás` row.

```text
Leer
Biblioteca        Añadir libro | Borrar libro
Grabadora         Nueva grabación | Enviar al móvil | Borrar grabación
Ajustes           Brillo | Salto por pulsación | Wi-Fi
Info              Firmware | Libre | Apagar: mantén SELECT
```

The five root rows scroll through the four visible OLED rows so the focus stays visible. Each screen's title is the name of the row that opens it, in capitals (`LEER`, `BIBLIOTECA`, `GRABADORA`, `AJUSTES`, `INFO`).

### Leer

Opens the list of stored books. Selecting a book starts RSVP at its saved byte position, if it has unfinished progress, or at the first word otherwise. A completed book starts from the beginning when selected again.

- Show book titles in storage order, with long titles truncated to one line.
- Keep a selectable `< Atrás` row fixed on the bottom row; books scroll in the three rows above it.
- If there are no books, show `Sin libros` and `Sube uno desde Biblioteca.` (split over two lines), and the selectable `< Atrás` row.
- On return from reading, preserve the current UTF-8-safe byte position as specified in [`ARCHITECTURE.md`](ARCHITECTURE.md).
- At end of book, offer `Releer` and `Volver al menú`.

### Biblioteca

```text
BIBLIOTECA
  Añadir libro >
  Borrar libro >
  < Atrás
```

**Añadir libro** starts the device's temporary Wi-Fi session when Wi-Fi is not configured as Apagado. The OLED shows the network name, a one-time PIN and the address to open on the phone (`Red: Leeloo`, `PIN: 1234`, `Abre 192.168.4.1`), with `< Atrás` to cancel (like a long press). It then shows a short success or error message. This document specifies only what the device displays; the mobile page and conversion workflow are out of scope.

- If Wi-Fi is Apagado, do not enable it implicitly. Show `Wi-Fi desactivado` and `Actívalo en Ajustes.` with a return action.
- If the access point cannot start, show `No se pudo iniciar el Wi-Fi.` and allow the user to return.
- In Automático mode, stop the access point after a successful upload, explicit cancellation, or five minutes without activity. The timeout is a design default to validate during integration.
- In Encendido mode, keep the access point available while the device is awake. Entering this screen shows the current session details.

**Borrar libro** opens the book list (`BORRAR LIBRO`). After a book is selected, require an explicit confirmation:

```text
BORRAR LIBRO
¿Borrar este libro?
<título>
  Cancelar
  Borrar
```

Focus `Cancelar` by default. On confirmation, delete the file and its saved reading position, then return to the updated list. If the operation fails, keep the file and show an error with a way back.

### Grabadora

The device records mono 16-bit WAV from the I2S microphone (wiring in [`mic-inmp441.md`](hardware-checklists/mic-inmp441.md)).

```text
GRABADORA              GRABADORA (with a recording)
  Nueva grabación >      Enviar al móvil >
  < Atrás                Borrar grabación >
                         < Atrás
```

- **Nueva grabación** opens the recording screen at once and asks the recorder to start. The screen shows the elapsed time over the limit, `Pausar`/`Reanudar` and `Detener`; the title reads `GRABANDO` or `PAUSA`. It leaves only when the recorder answers: `Grabación guardada.` or an error message. A long SELECT press stops and saves like `Detener`.
- Capture is always 16 kHz mono. The length is not fixed: it is the whole seconds that fit in the free space after the 2 MiB reserve, up to 10 minutes. The Grabadora screen shows it before starting (`Disponible: 01:40`) and the recording screen counts up against it (`00:12 / 01:40`). With less than 5 seconds available the screen says `Sin espacio. Libera espacio para grabar.`; books are never deleted automatically. The capture ends at the limit and saves what it has, and a full disk or a microphone fault mid-recording saves what was captured so far.
- Paused time does not count towards the limit. A recording left paused for 5 minutes is saved.
- Only one recording can be stored. With one stored, the menu offers `Enviar al móvil` and `Borrar grabación` instead of `Nueva grabación`. If another storage operation is running, `Almacenamiento ocupado. Reintenta.` is shown.
- **Enviar al móvil** starts the same authenticated Wi-Fi session as Añadir libro (title `ENVIAR AL MÓVIL`), with `< Atrás` to cancel. With Wi-Fi Apagado it shows `Wi-Fi desactivado` instead. The phone page receives, plays and offers to download the WAV; the device deletes its copy only after the phone confirms the exact byte count (see [`WEB-API.md`](WEB-API.md) §3.4) and then says `Grabación enviada.` An interrupted transfer leaves the recording for another try.
- **Borrar grabación** asks `¿Borrar la grabación?` with `Cancelar` focused. A failure keeps the file and says so.
- If the phone confirms a transfer while another screen is open, the recording simply stops being stored.

Transcription and summary are done on the phone, never on the device (the page that receives the recording offers them, `ARCHITECTURE.md` §5.3). A transcript is separate data and must not be deleted with the WAV.

### Ajustes

```text
AJUSTES
  Brillo >
  Salto por pulsación >
  Wi-Fi >
  < Atrás
```

Each setting screen ends with a selectable `< Atrás` row that returns to Ajustes.

**Brillo** is one 10–100 % level, adjustable in 5 % steps (`Nivel < 80% >`). The SH1106 has no separate backlight, so the level drives its contrast register: `round(255 × level / 100)`. There is no ambient light sensor, so there is no automatic mode. 100 % is the default. Confirm the useful range on the actual module.

**Salto por pulsación** (title `SALTO POR PULSACIÓN`, row `Salto`) selects how much UP/DOWN change the reading speed per press while reading: 5, 10, 25, 50 or 100 PPM, 25 by default. A line below explains it: `Cambia la velocidad en cada pulsación.` The speed stays within 100–800 PPM and aligns to multiples of the step counted from 100, so both bounds are always reachable. The current speed itself is changed while reading, not in the menu, and is remembered across books.

**Wi-Fi** cycles through three persistent choices, each explained by two lines under the value:

| Label | Device behavior | Help shown |
|---|---|---|
| `Apagado` | Keep the access point off. Añadir libro and Enviar al móvil ask the user to enable Wi-Fi in Ajustes. | `No se puede enviar ni recibir nada.` |
| `Automático` | Keep Wi-Fi off until a session is started; stop it on success, cancellation, or inactivity. | `Se activa solo al añadir o enviar.` |
| `Encendido` | Keep the access point available while the device is awake. | `Siempre activo. Gasta más batería.` |

SELECT starts editing; UP/DOWN change the value and SELECT ends editing. Generate a different PIN for each session. Deep sleep always stops Wi-Fi; after wake, apply the saved mode again.

### Info

A read-only `INFO` screen with the firmware version (`Firmware`), free storage (`Libre`) and a reminder of how to switch off (`Apagar: mantén SELECT`), plus `< Atrás`. It never shows session PINs or other credentials. Battery is not shown until the firmware can measure it; omit unavailable values rather than showing fabricated data.

### Apagar

Holding SELECT (about 0.8 s) on the root menu opens `APAGAR` with the question `¿Apagar Leeloo?` and two rows, `No` (focused) and `Sí, apagar`. `No` or another long press goes back. `Sí` shows `Apagando...` for a moment, stops Wi-Fi, switches the panel off, closes the filesystem and puts the device into deep sleep. The reading position and the settings are already saved by then (the root menu is only reachable with no book open and nothing recording).

To switch it on again, hold SELECT: the device wakes on the press and goes back to sleep unless the button is still held after about 1 s in total, so a knock in a bag does not switch it on. Wi-Fi comes back as the saved mode says. This is deep sleep, not a power switch: the development board keeps drawing a little current through its regulator and USB chip (see `hardware-checklists/power.md`).

## 3. Navigation and feedback

One rule set for every screen:

- UP/DOWN move the focus or change a focused value; lists wrap at both ends and holding does not repeat in menus.
- SELECT short press selects the focused row, including `< Atrás`, which returns to the parent.
- **A long press (about 0.8 s) always goes back one level.** Where there is nothing to go back to it does the most useful thing: on the root menu it asks to switch the device off; while reading it returns to the book list; while recording it stops and saves; on the Wi-Fi screens it cancels the session.
- Setting screens: SELECT short starts editing the value (shown as `[ v ]` instead of `< v >`), UP/DOWN change it, and the next SELECT short or long press ends the edit.
- Message screens (errors and confirmations) end with `Pulsa para volver`; any press returns to the screen they came from.
- Keep focus visible through inversion. Do not rely on icons or color alone.
- Deletion is the only menu action that removes user data; it always requires a second, explicit confirmation.
- Persist brightness, Wi-Fi mode, reading-speed step, last global reading speed, and per-book reading position. Apply setting changes immediately and retain them across restart.
- Use brief Spanish status/error messages and keep a clear return path. Empty libraries, storage-full conditions and upload failures must not look like successful operations.

## 4. OLED layout

- Use the shared 128x64 grid, margins, type, and inverted focus treatment from [`DESIGN.md`](DESIGN.md).
- Keep each menu item on one line; truncate long titles with `...`. Message and help lines are at most 21 characters (`lib/ui/ui_text.h` enforces this in tests).
- Keep `< Atrás` fixed on the bottom row of every screen that shows it. Scroll long lists within the three rows above it so the focused item remains visible. No horizontal text scrolling and no scroll indicators. Between menu screens use the short slide in [`DESIGN.md`](DESIGN.md) §3.4, never into or out of RSVP reading.
- Keep live speed adjustment in the RSVP screen; configure its step in Ajustes > Salto por pulsación.

## 5. Open hardware checks

- Verify the OLED controller and the useful brightness range (register values 26–255).
- Validate the Wi-Fi inactivity timeout and upload completion/cancel behavior on the device.
- Confirm which battery and free-space values can be reported reliably.

## 6. Implementation decisions (first firmware pass)

Choices the spec left open, recorded so the hardware checks can confirm them:

- **Input:** three buttons (UP GPIO5, DOWN GPIO4, SELECT GPIO6; wiring in `docs/hardware-checklists/buttons-input.md`, pins in `src/pins.h`), not an encoder.
- **Brillo:** one control that writes the single SH1106 contrast register. An earlier version had a separate Contraste and an Auto/Manual mode; both were removed as redundant (no light sensor). Old stored values are ignored.
- **Wi-Fi default:** `Automático`. The spec does not set one.
- **Battery:** not shown. The firmware cannot measure it yet.
- **Reading controls:** while reading, UP and DOWN change the speed by the configured step (5/10/25/50/100, range 100–800), and `N PPM` shows in the top-left for about 2 s after a change; holding repeats after 400 ms and every 150 ms. SELECT short pauses; SELECT long returns to the list. While paused, `PAUSA` shows, UP and DOWN jump one sentence per press (holding does not repeat), and SELECT short resumes. The speed is global across books (300 PPM on a new device) and is saved when leaving or finishing a book, not on pause. The position is saved on pause and on leaving the book, not continuously. Checklist: `docs/hardware-checklists/reading.md`.
- **Firmware version:** `0.1.0-dev`, a constant in `src/display_ui.cpp`.
- **Font:** `u8g2_font_6x10_tf`. The ellipsis is written as `...` because the glyph for `…` is unverified in that font. Confirm on the panel.
- **List window:** four rows, scrolled by `lib/text` so the cursor stays visible, with wrap-around at both ends.
- **Storage:** LittleFS is mounted without formatting. A blank or corrupt partition shows `Almacenamiento no disponible` and the books are untouched. Flash the filesystem image before the first test.
- **Replace and power loss:** a replaced book is moved aside before the new one is renamed in. A power cut between those two renames loses the old book. Known limit, to revisit if it matters on hardware.
- **Automático after a save:** the AP stays up for 1.5 s so the phone receives its `201`, then stops.
