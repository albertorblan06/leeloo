# Upload web API contract

Contract between the phone web app (`web/`) and the ESP32 firmware for the
temporary Wi-Fi session: uploading a book and receiving a recorded WAV. Source of
truth for endpoint shapes, error codes and limits. Firmware constants must match
section 4.

Transport: plain HTTP over the device soft AP (no TLS, no internet, no external
hosts). The page never loads anything from outside the AP.

## 1. Session lifecycle

- The device starts the soft AP and generates a new 4-digit PIN only when the
  user selects Upload Book or `Grabadora > Enviar al móvil`.
- The PIN is shown on the OLED. The web page must send it before any upload.
- A successful PIN check opens the session for that client.
- The session ends when the user exits the Wi-Fi screen, long-presses to cancel, or
  the AP shuts down (in Automático mode also after five minutes without requests).
  After that every request needs a new session.
- No credentials are stored. The PIN is discarded when the session ends.

## 2. Static assets

`npm run build` in `web/` writes `web/dist/<path>.gz`; that tree is the LittleFS
image content. `GET /` maps to `index.html`. For any other `GET <path>`, the
device serves the file `<path>.gz` with `Content-Encoding: gzip` and a
`Content-Type` from the extension (`.html`, `.css`, `.js` and `.mjs` as
`text/javascript`). Unknown paths return `404`.

| Path | Content |
|---|---|
| `/`, `/index.html` | page |
| `/app.js`, `/style.css` | glue and styles |
| `/src/*.js` | tested ES modules imported by `app.js` |
| `/vendor/pdf.mjs`, `/vendor/pdf.worker.mjs` | pdf.js 6.4.299 legacy build (minified) |

The page offers both actions after the PIN: upload a book, or receive the audio.
For audio it downloads the WAV into a Blob, plays it and offers a download link.

Size must be re-measured with `npm run build` (last measured before the audio
page: 548 817 bytes gzip, 541 814 of them pdf.js). No CDN, no external fonts.

## 3. Endpoints

### 3.1 `POST /api/auth`

Request: `Content-Type: application/json`

```json
{ "pin": "1234" }
```

- `200` `{}` and a session cookie `leeloo_session=<opaque>; HttpOnly; SameSite=Strict; Path=/`
- `401` `invalid_pin`
- `410` `session_closed` — reserved; the firmware does not emit it (see §6)

### 3.2 `POST /api/books`

Requires a valid session cookie.

Headers:

- `Content-Type: text/plain; charset=utf-8`
- `X-Book-Name: <percent-encoded UTF-8 filename>` — the name after sanitization
  must be at most 48 bytes. The web page strips the extension and sends the
  name; the device sanitizes it again and never trusts it for paths.

Body: the complete normalized `.txt` file (format in `docs/ARCHITECTURE.md` §5.1):
LF line endings, UTF-8, header, `###START###`, then the words.

- `201` `{ "id": "<device-assigned id>", "bytes": <n> }`
- `400` `bad_name` — missing, empty after sanitization, or over 48 bytes
- `401` `unauthorized` — no or unknown session cookie
- `409` `busy` — another long storage operation (a capture, an upload) is running
- `409` `library_full` — 20 books already stored
- `410` `session_closed`
- `413` `too_large` — body over 1 MiB (1 048 576 bytes)
- `422` `invalid_utf8`
- `422` `invalid_format` — header missing, `###START###` missing or not on its own line, or empty body after the delimiter
- `500` `storage_error` — the temp-file write or the final rename failed; nothing was stored
- `507` `no_space` — free space would drop below the reserve

A book whose sanitized name already exists replaces it. The old file is moved
aside, the new one is renamed into place, and only then is the old one deleted;
if the rename fails the old book is restored. It does not count toward
`library_full`.

Firmware writes the body to a temporary file, validates it, then renames it into
place (atomic). Incomplete temporary files are removed at boot.

### 3.3 `GET /api/recording`

Requires a valid session cookie. Returns the pending WAV:

- `200` `Content-Type: audio/wav`, `Content-Disposition: attachment; filename="grabacion.wav"`.
  The body is chunked, so there is no `Content-Length`: the page checks the file against
  its WAV header instead (44 bytes plus exactly the declared data length, consistent RIFF size).
- `401` `unauthorized`
- `404` `no_recording`
- `500` `storage_error` — the stored file is not a valid WAV of this firmware

The endpoint never deletes the file, including when the client disconnects.

### 3.4 `POST /api/recording/ack`

Requires a valid session cookie. The page calls it only after the whole WAV was
received and checked.

Body: `Content-Type: application/json`, `{ "bytes": <bytes received> }` (a non-negative
integer of at most 9 digits).

- `200` `{ "deleted": true }` — the byte count equals the stored size and the device removed its copy. Also returned when no WAV remains (idempotent). In Automático mode the AP then stops after a short grace period.
- `400` `invalid_ack` — missing or malformed body
- `401` `unauthorized`
- `409` `size_mismatch` — the count differs from the stored size; the WAV is kept
- `409` `busy` — another long storage operation is running
- `500` `storage_error` — the WAV could not be removed

### 3.5 Not a device endpoint: transcription

The device has no endpoint for transcription or summary. The page does it by calling Groq directly
(`docs/ARCHITECTURE.md` §5.3), so nothing about it crosses the device's API.

## 4. Limits

| Constant | Value | Notes |
|---|---|---|
| `kMaxBookBytes` | 1 048 576 | normalized book, after conversion |
| `kMaxBooks` | 20 | subject to partition capacity |
| `kMaxNameBytes` | 48 | after UTF-8-safe sanitization |
| `kReserveBytes` | 2 097 152 | firmware/recovery reserve |
| `kPinDigits` | 4 | generated per session |
| `kWriteSlackBytes` | 16 384 | extra LittleFS headroom for WAV writes |
| WAV format | PCM mono 16-bit, 16 kHz | length from the free space at start, 5 s to 600 s |
| Maximum WAV size | 19 200 044 bytes | 600 s plus the 44-byte header; 8 kHz files from earlier firmware are still served |

The web page enforces the same size and name limits before sending, so most
errors are caught client-side. The device still enforces all of them.

## 5. Error body

All non-2xx responses use the same shape:

```json
{ "error": "invalid_format" }
```

The web page maps `error` codes to Spanish messages. Unknown codes show a
generic error. A failed book upload leaves the book unsent; a failed audio
transfer leaves the WAV on the device.

## 6. Open points

- `session_closed` is not emitted by the firmware. Ending the session turns off
  the AP, so the page sees a network error and shows that message instead.
  The client keeps the mapping in case a later firmware returns the code.
- Partition size: `partitions.csv` gives LittleFS 6 MiB, above the 2 MiB reserve.
  It assumes 16 MB flash and is unverified until the board is checked.
- An early error (401, 400, 409, 413, 422 mid-stream, 507) drains the unread
  body first, bounded by 1 MiB, so the phone receives the JSON error code.

- PIN attempts: decided — no lockout and no attempt counter. The AP is
  short-lived, local and user-initiated, so retries are unlimited.
- A recording needs its WAV size, the 2 MiB reserve and the write margin free. If
  neither format fits, the device refuses to start and asks the user to free space.
- The ack follows a successful browser download, not the user finishing the phone's
  final "save to Downloads" prompt. The page plays the audio from memory, so the
  user can still save it from there, but closing the page before that loses it.
- Upload progress: the browser uses `XMLHttpRequest.upload` progress events.
  Confirm this works on iOS Safari with the AP.
