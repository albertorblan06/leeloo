# Voice recording (Grabadora)

Status: **written, not yet run on hardware.** Spec: `docs/DEVICE-MENU.md` "Grabadora", `docs/WEB-API.md` §3.3–3.4.
Wiring and the bare microphone check are in [`mic-inmp441.md`](mic-inmp441.md): do that first. This checklist runs on the
production firmware (`pio run -e esp32s3 -t upload`), not on the `mic-test` image.

## Before
- [ ] `mic-inmp441.md` passes (the peak follows sound).
- [ ] The filesystem image has the new web page: `npm run build` in `web/`, then rewrite the LittleFS image at `0x670000`.
- [ ] Record the free space before starting (INFO shows it): ______ KB.

## Capture
- [ ] The root menu shows five rows (Leer, Biblioteca, Grabadora, Ajustes, Info) and scrolls so the focus stays visible. Info is reachable.
- [ ] Grabadora > Nueva grabación shows `Disponible: mm:ss` first, then opens `GRABANDO` with `00:00 / <that time>`; the time counts up once per second. The available time matches the free space in INFO: about (free KB − 2112) / 31.25 seconds. Record both: ______.
- [ ] Speak, then Pausar: the title reads `PAUSA`, the time stops, `Reanudar` appears. Wait 10 s, Reanudar, speak again, Detener. The saved audio does not contain the 10 s gap (check on a computer).
- [ ] Detener shows `Grabación guardada.` and the menu then offers `Enviar al móvil` and `Borrar grabación`. A long SELECT press does the same.
- [ ] Check the file from the phone page: a WAV, mono, 16-bit, 16 kHz, about the recorded length. Record whether the **level** is usable or too quiet: ______ (the conversion keeps the top 16 of the 24 bits; add gain only if it is too quiet).
- [ ] Clap at a steady tempo (about 2 per second) for 30 s. The recording must last as long as the clock says and have no gaps, repeated pieces or speed-up (16 kHz is the demanding rate). If it has, the DMA buffer or the LittleFS write stalls need work (`kDmaBuffers` in `src/audio_adapters.cpp`).
- [ ] **The stored WAV is servable**: record, Detener, then Enviar al móvil and receive it. The page must report a complete file (not `La grabación recibida está incompleta`). Do the same after letting a recording run to the limit and after a full disk. A WAV whose header disagrees with its size would answer 500 for ever; the firmware now measures the closed file, patches the header from it and checks it before renaming, so a failure shows as `No se guardó la grabación` instead.
- [ ] Let it run to the limit: it stops by itself at the limit shown, saves, and the WAV plays back completely. Note the real length against the predicted one: ______ s vs ______ s (LittleFS counts whole blocks, so the prediction is approximate; if the disk fills early the recording is still saved).
- [ ] Pause and leave it: after 5 minutes it saves by itself and the storage is free again (a book upload no longer says busy).
- [ ] With a WAV pending, the menu no longer offers Nueva grabación.
- [ ] Add books until `Disponible` drops below 5 s: the Grabadora screen says `Sin espacio. Libera espacio para grabar.`, Nueva grabación shows `Sin espacio.` and no book or WAV is touched. Delete a book: the time grows.
- [ ] Fill the disk on purpose during a recording (upload a large book from the phone is refused while recording, so add books first to leave ~10 s): the recording is saved when the disk fills and the WAV plays back.
- [ ] Remove power during a capture: after reboot the pending WAV is absent (the temporary file is gone) and a new recording starts normally.
- [ ] Disconnect the microphone SD line and record: the capture does not hang; it gives a clear result (silence or an error), and the menu keeps working.

## Delete
- [ ] Borrar grabación focuses `Cancelar`; Cancelar keeps the file. Borrar removes it and the menu offers Nueva grabación again.
- [ ] Reboot with a WAV pending: it is still listed.

## Transfer
- [ ] Enviar al móvil shows `Red: Leeloo`, the PIN and `Abre 192.168.4.1` with `< Atrás`. With Wi-Fi Apagado it says `Wi-Fi desactivado` instead.
- [ ] On the phone: join the AP, enter the PIN, choose `Recibir la grabación`. The recording plays and can be downloaded, the page says it was deleted from the device, and the OLED shows `Grabación enviada.` and then offers Nueva grabación again.
- [ ] Interrupt the download (airplane mode mid-transfer): the WAV is still on the device and a second attempt works.
- [ ] Choose the audio with nothing pending: the page says there is no audio pending.
- [ ] While a recording is running (Wi-Fi mode Encendido), a book upload from the phone is refused with a busy message and nothing is lost.
- [ ] With Wi-Fi Encendido, confirm a transfer while the device is on another screen (reading, or the root menu): the menu stops offering Enviar al móvil afterwards.
- [ ] `< Atrás` on the Enviar al móvil screen cancels and the WAV stays.

## Transcription and summary (phone page)
Needs a Groq key (console.groq.com) and mobile data on the phone.
- [ ] After receiving a recording, the page shows `Transcribir y resumir`, the privacy line and an empty `Clave de Groq` field.
- [ ] Without a key, the button says `Pega tu clave de Groq para transcribir.` and nothing is sent.
- [ ] A wrong key says `Groq no acepta esa clave…`.
- [ ] **With mobile data on and the phone joined to `Leeloo`** (the critical test): it transcribes and summarizes. Record the phone, OS and result: ______. If the request fails with `No hay internet`, note whether turning Wi-Fi's "switch to mobile data" / "keep connection" option on or off changes it: ______.
- [ ] With mobile data off it says `No hay internet. Activa los datos móviles…`, the recording is still playable and downloadable, and the device copy was already handled as before.
- [ ] The transcript is in correct Spanish for a clear 20 s sample, and the summary is a few bullets in Spanish. Quality of the 16 kHz recording for speech: ______.
- [ ] Reload the page: the key is still there (`Olvidar la clave` clears it); the received recording and its transcript are gone, as the page warns (download first). Receive another recording: the previous notes do not appear.
- [ ] `Copiar` puts both texts on the clipboard (the page is plain http, so this uses the fallback); `Descargar .txt` saves `grabacion.txt` with the summary first.
- [ ] A recording near the limit (~10 min, 19 MB) is accepted by Groq, or the page says it is too large.
- [ ] The key does not appear in the address, in the page source or in the device's serial log.
- [ ] With site data or cookies blocked in the phone browser, the page still loads and transcribes (the key is just not remembered).

## Power
- [ ] Repeat the battery brownout test of `battery-tp4056.md` with the AP up **and** a recording running (the worst case). Record the lowest voltage seen and whether the board resets: ______.
