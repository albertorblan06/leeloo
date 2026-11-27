# On-device menu (three buttons)

Status: **written, not yet run on hardware.** Spec: `docs/DEVICE-MENU.md`. Choices the spec left open: §6 of that document.

## Before flashing
- [ ] `esptool --port /dev/cu.usbmodem5CF80494941 flash_id` reports 16 MB. If not, stop: the partition table in `partitions.csv` must change first.
- [ ] Filesystem image built in OrbStack `ubuntu-x86` from `web/dist` and written at offset `0x670000`. Record the image size: ______ bytes.
- [ ] Firmware flashed with `pio run -e esp32s3 -t upload`.

## Boot
- [ ] The boot logo plays: a dot with a guide above; a second guide turns clockwise from the top, through the right, to the bottom; `leeloo` types out letter by letter to the right; the dot beats once; the logo rests, then the main menu appears. Total about 2 s: measure ______ s.
- [ ] The animation is smooth (no visible stutter). Count the frames per second by video: ______ fps. If it is below ~15 fps, lower the I2C dump cost or draw fewer frames.
- [ ] The mark and the word are centred together on the panel, nothing is clipped at either edge, and the word is legible (the `u8g2_font_helvR14_tr` lowercase letters). Note anything to adjust (`kCx`, `kWordX` in `src/display_ui.cpp`): ______.
- [ ] The beat is visible but not jumpy (the dot grows from 8 to 9 px and back).
- [ ] With the OLED disconnected the firmware still boots with no delay from the animation.
- [ ] A slow start (a large filesystem to mount) does not stretch the animation: it skips frames and ends on time.
- [ ] The main menu shows Leer, Biblioteca, Grabadora, Ajustes on evenly spaced rows with no title, divider or icons; the first row is focused and Info appears when scrolling down.
- [ ] Serial prints no `LittleFS no montó`. If it does, the menu shows `Almacenamiento no disponible` and no book is touched.
- [ ] If the saved Wi-Fi mode is `Encendido`, the AP `Leeloo` appears after boot.

## Navigation
- [ ] Wiring as in `buttons-input.md`: UP GPIO4, DOWN GPIO5, SELECT GPIO6, each to GND. Serial prints ARRIBA, ABAJO or SELECT on each press.
- [ ] UP and DOWN move the cursor and wrap at both ends.
- [ ] SELECT short opens each screen; SELECT long goes back one level from each sub-screen.
- [ ] Every screen with a parent (Leer, Biblioteca, Borrar libro, Ajustes, Brillo, Salto por pulsación, Wi-Fi, Info, Añadir libro, Grabadora) shows `< Atrás` on the bottom row; selecting it goes to the parent. Lists have no scroll bar and the focused row stays visible as they scroll.
- [ ] With no books, Leer and Borrar libro show `Sin libros` and a focused `< Atrás`.
- [ ] SELECT short on the Añadir libro screen cancels the session like SELECT long.
- [ ] Moving forward slides the new screen in from the right; going back slides it in from the left. Measure one transition with a stopwatch or video: about ______ ms. RSVP entry/exit and word updates are not animated.
- [ ] No screen is drawn with a color cue; the focus is the inverted bar only.
- [ ] All Spanish strings show accents correctly: á é í ó ú ñ ü ¿ ¡. If the `...` ellipsis or any accent is missing, record it in DEVICE-MENU §6.

## Ajustes (persistence)
- [ ] Editing: SELECT short marks the value as `[ v ]`, UP raises and DOWN lowers it, SELECT short or SELECT long ends the edit without leaving the screen.
- [ ] BRILLO has one row, Nivel, in 5 % steps between 10 % and 100 %; the panel gets visibly dimmer and brighter. Record the lowest level that is still readable: ______ %. There is no Modo row and no `Sensor no disponible`.
- [ ] WI-FI cycles Apagado → Automático → Encendido and back.
- [ ] AJUSTES lists Brillo, Salto por pulsación and Wi-Fi, then `< Atrás`. SALTO POR PULSACIÓN offers 5, 10, 25, 50 and 100 PPM as `Salto`, with 25 by default, and explains itself in two lines.
- [ ] WI-FI shows a two-line help under the mode that changes with it (Apagado, Automático, Encendido).
- [ ] Holding UP/DOWN while editing Brillo or Salto repeats after 400 ms and every 150 ms. Menu navigation and the Wi-Fi mode do not repeat.
- [ ] Reset the board after changing each value: the same value is shown after boot.

## Known limits to watch
- [ ] SELECT long during an upload stops the AP from the menu task. `httpd_stop` may wait for the transfer in flight, so the screen can freeze for a moment. Measure how long: ______ s.
- [ ] After a flash, `/books` is created at boot. Serial must not print `/littlefs/books does not exist`.
- [ ] With the OLED disconnected the firmware still boots and prints `No OLED at 0x3C/0x3D`. Wiring: SDA GPIO8, SCL GPIO10, VCC 3V3.

## Biblioteca and Wi-Fi
- [ ] Añadir libro with Wi-Fi Apagado shows `Wi-Fi desactivado`, and no AP appears.
- [ ] Añadir libro with Automático shows `Red: Leeloo`, the PIN and `Abre 192.168.4.1`. The PIN is fully visible.
- [ ] A save from the phone shows `Libro guardado.`, the book appears under Leer, and the AP disappears about 1.5 s later.
- [ ] Automático: no upload activity for 5 minutes shows `Conexión cerrada`, and the AP stops.
- [ ] Encendido: after a save the AP stays up; SELECT long leaves the screen without stopping the AP.
- [ ] Cancel with SELECT long during an upload stops the AP in Automático.

## Leer and Borrar libro
- [ ] With no books: `Sin libros` and the two help lines (`Sube uno desde` / `Biblioteca.`).
- [ ] Selecting a book shows `La lectura aún no está disponible.` and returns to the list.
- [ ] Borrar libro: the confirmation focuses Cancelar. Cancelar keeps the file.
- [ ] Borrar removes the file and the list updates. The file is gone from the filesystem after reboot.
- [ ] Two titles longer than the row are cut with `...` and never split a letter.
- [ ] Every new label fits one row without clipping: `¿Borrar este libro?`, `No se pudo abrir el libro`, `Sube uno desde`, `Sin libros`.

## Info
- [ ] Shows the firmware version, free storage in KB and `Apagar: mantén SELECT`, then `< Atrás`. No battery value is shown.

## Measurements
| Measure | Value |
|---|---|
| Free heap at idle with the menu running | |
| Free heap during an upload | |
| Time from SELECT to a redraw | |
