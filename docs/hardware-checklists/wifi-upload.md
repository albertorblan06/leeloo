# Wi-Fi soft AP and book upload

Status: **written, never run on hardware.** The adapter is `src/upload_server.cpp`; it compiles for `esp32s3` but is not yet wired into the menu (Milestone 7). Contract: `docs/WEB-API.md`.

## Before you start (blockers found while writing this)
- [x] **Partition table.** `partitions.csv` gives LittleFS (`spiffs`) 6.00 MiB, enough for the 2 MiB reserve. Assumes 16 MB flash (`board_upload.flash_size = 16MB`, from the N16R8 name). Verify the real flash size on the board before flashing.
- [ ] Build the web app (`cd web && npm run build`), then flash the filesystem. **Known problem:** `tool-mklittlefs` in PlatformIO is an x86 binary and fails on this arm64 Mac (`Bad CPU type`), so `-t buildfs` / `-t uploadfs` do not run here. Use Rosetta or a Linux host and record the LittleFS image size: ______ bytes.
- [ ] Temporary wiring: call `leeloo::upload_server::start(pin)` from a test sketch and show `pin` on the OLED. Remove it after Milestone 7 integrates the real menu entry.

## Session start
- [ ] `start()` returns true and a Wi-Fi network named `Leeloo` appears on the phone.
- [ ] A new PIN is shown each time `start()` is called (check three starts in a row).
- [ ] The phone joins the AP and `http://192.168.4.1/` loads the page (iOS Safari and Android Chrome).
- [ ] The page loads with no internet: no request leaves the AP (check the browser network panel for any external host).
- [ ] If LittleFS fails to mount, `start()` returns false and the existing books are untouched (never auto-formats).

## PIN and session
- [ ] A wrong PIN shows "PIN incorrecto" and can be retried without limit.
- [ ] The right PIN moves to the file picker; the response sets `leeloo_session` with `HttpOnly` and `SameSite=Strict`.
- [ ] An upload without the cookie (for example `curl -X POST http://192.168.4.1/api/books`) returns 401 `unauthorized`.
- [ ] After `stop()` the Wi-Fi network disappears and the old cookie no longer works after a new `start()`.

## Conversion in the phone
- [ ] A text PDF of about 100 pages converts, with progress moving and Cancel working mid-way.
- [ ] A scanned (image-only) PDF shows the "no tiene texto seleccionable" message and nothing is sent.
- [ ] A password-protected PDF shows the "contraseña" message.
- [ ] Spanish characters (á é í ó ú ñ ¿ ¡) survive: open the stored book and read them on the OLED.
- [ ] Words split by a hyphen at a line end are rejoined; a book over 1 MiB of text is refused before sending.

## Upload and storage
- [ ] Upload a small book: the page says sent, `/books/<name>.txt` exists, the byte count matches the response `bytes`.
- [ ] Upload a book near 1 MiB: completes, and RAM stays stable (watch free heap before and after; the body is streamed in 1 KiB chunks).
- [ ] Upload the same title twice: the book is replaced, the library count does not grow.
- [ ] Fill the library to 20 books: the 21st returns `library_full`.
- [ ] Fill the flash until the reserve is reached: the next upload returns `no_space` and no partial file remains.
- [ ] Cut phone Wi-Fi mid-upload: no `.upload.tmp` stays after the next `start()`.
- [ ] Power-cycle mid-upload: after boot the library shows only complete books and the temp file is gone.
- [ ] A book with a missing `###START###` or invalid UTF-8 (craft one with curl) is rejected with 422 and not stored.

## Measurements to record
| Measure | Value |
|---|---|
| Upload time, 100 KiB book | |
| Upload time, 1 MiB book | |
| Free heap with server running | |
| Reading a book while the AP is up (no stutter) | |
