# On-device menu (three buttons)

Status: **written, not yet run on hardware.** Spec: `docs/DEVICE-MENU.md`. Choices the spec left open: §6 of that document.

## Before flashing
- [ ] `esptool --port /dev/cu.usbmodem5CF80494941 flash_id` reports 16 MB. If not, stop: the partition table in `partitions.csv` must change first.
- [ ] Filesystem image built in OrbStack `ubuntu-x86` from `web/dist` and written at offset `0x670000`. Record the image size: ______ bytes.
- [ ] Firmware flashed with `pio run -e esp32s3 -t upload`.

## Boot
- [ ] The OLED shows the LEELOO screen with four rows: Leer, Biblioteca, Ajustes, Info. The first row is focused.
- [ ] Serial prints no `LittleFS no montó`. If it does, the menu shows `Almacenamiento no disponible` and no book is touched.
- [ ] If the saved Wi-Fi mode is `Encendido`, the AP `Leeloo` appears after boot.

## Navigation
- [ ] Wiring as in `buttons-input.md`: UP GPIO4, DOWN GPIO5, SELECT GPIO6, each to GND. Serial prints ARRIBA, ABAJO or SELECT on each press.
- [ ] UP and DOWN move the cursor and wrap at both ends.
- [ ] SELECT short opens each screen; SELECT long goes back one level from each sub-screen.
- [ ] No screen is drawn with a color cue; the focus is the inverted bar only.
- [ ] All Spanish strings show accents correctly: á é í ó ú ñ ü ¿ ¡. If the `...` ellipsis or any accent is missing, record it in DEVICE-MENU §6.

## Ajustes (persistence)
- [ ] Editing: SELECT short marks the value as `[ v ]`, UP raises and DOWN lowers it, SELECT short or SELECT long ends the edit without leaving the screen.
- [ ] BRILLO: Modo toggles Auto/Manual; in Auto the Nivel row disappears and `Sensor no disponible` shows.
- [ ] Nivel changes in 5 % steps between 10 % and 100 %, and the panel gets dimmer and brighter visibly.
- [ ] CONTRASTE changes in 5 % steps and the panel responds. Record the visible range: ______.
- [ ] WI-FI cycles Apagado → Automático → Encendido and back.
- [ ] Reset the board after changing each value: the same value is shown after boot.

## Known limits to watch
- [ ] SELECT long during an upload stops the AP from the menu task. `httpd_stop` may wait for the transfer in flight, so the screen can freeze for a moment. Measure how long: ______ s.
- [ ] After a flash, `/books` is created at boot. Serial must not print `/littlefs/books does not exist`.
- [ ] With the OLED disconnected the firmware still boots and prints `No OLED at 0x3C/0x3D`. Wiring: SDA GPIO8, SCL GPIO9, VCC 3V3.

## Biblioteca and Wi-Fi
- [ ] Subir with Wi-Fi Apagado shows `Wi-Fi desactivado`, and no AP appears.
- [ ] Subir with Automático shows SSID, PIN and address. The PIN is fully visible.
- [ ] A save from the phone shows `Libro guardado.`, the book appears under Leer, and the AP disappears about 1.5 s later.
- [ ] Automático: no upload activity for 5 minutes shows `Carga cerrada`, and the AP stops.
- [ ] Encendido: after a save the AP stays up; SELECT long leaves the screen without stopping the AP.
- [ ] Cancel with SELECT long during an upload stops the AP in Automático.

## Leer and Borrar
- [ ] With no books: `Sin libros` and the two help lines (`Sube uno desde` / `Biblioteca.`).
- [ ] Selecting a book shows `La lectura aún no está disponible.` and returns to the list.
- [ ] Borrar: the confirmation focuses Cancelar. Cancelar keeps the file.
- [ ] Borrar removes the file and the list updates. The file is gone from the filesystem after reboot.
- [ ] Two titles longer than the row are cut with `...` and never split a letter.
- [ ] Every new label fits one row without clipping: `¿Borrar este libro?`, `No se pudo abrir el libro`, `Sube uno desde`, `Sin libros`.

## Info
- [ ] Shows the firmware version, free storage in KB, and the Wi-Fi mode. No battery value is shown.

## Measurements
| Measure | Value |
|---|---|
| Free heap at idle with the menu running | |
| Free heap during an upload | |
| Time from SELECT to a redraw | |
