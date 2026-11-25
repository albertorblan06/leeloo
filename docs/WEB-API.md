# Upload web API contract

Contract between the phone web app (`web/`) and the ESP32 firmware for book
upload and WAV reception. Source of truth for endpoint shapes, error codes and
limits. Firmware constants must match section 4.

Transport: plain HTTP over the device soft AP (no TLS, no internet, no external
hosts). The page never loads anything from outside the AP.

## 1. Session lifecycle

- The device starts the soft AP and generates a new 4-digit PIN when the user
  selects Upload Book or `Grabar > Subir audio`.
- The PIN is shown on the OLED. The web page must send it before any upload.
- A successful PIN check opens the session for that client.
- The session ends when the user exits the active Wi-Fi screen, long-presses to
  cancel, or the AP shuts down. In Auto mode, inactivity also closes it. After
  that every request requires a new session.
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

After PIN authentication the page offers both book upload and audio reception.
For audio, it downloads the WAV into a browser Blob, then exposes playback and a
download link. This lets the device delete its copy only after the browser has
received all bytes.

Measured size: 551 485 bytes gzip in total (pdf.js is 543 582 of those). No CDN,
no external fonts.

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
- `409` `library_full` — 20 books already stored
- `410` `session_closed`
- `413` `too_large` — body over 1 MiB (1 048 576 bytes)
- `422` `invalid_utf8`
- `422` `invalid_format` — header missing, `###START###` missing or not on its own line, or empty body after the delimiter
- `500` `storage_error` — the temp-file write or the final rename failed; nothing was stored
- `507` `no_space` — free space would drop below the reserve
- `409` `busy` — another long-running LittleFS mutation is active

A book whose sanitized name already exists replaces it. The old file is moved
aside, the new one is renamed into place, and only then is the old one deleted;
if the rename fails the old book is restored. It does not count toward
`library_full`.

Firmware writes the body to a temporary file, validates it, then renames it into
place (atomic). Incomplete temporary files are removed at boot.

### 3.3 `GET /api/recording`

Requires a valid session cookie. Returns the complete pending WAV:

- `200` `Content-Type: audio/wav`, attachment filename `grabacion.wav`
- `401` `unauthorized`
- `404` `no_recording`
- `500` `storage_error` — the stored recording is invalid or could not be read

The endpoint never deletes the file, including when the client disconnects
mid-transfer.

### 3.4 `POST /api/recording/ack`

Requires a valid session cookie. The browser calls this only after its full
`GET /api/recording` response has been consumed into a Blob.

- `200` `{ "deleted": true }`; the device removes the WAV
- `401` `unauthorized`
- `409` `busy` — another LittleFS mutation is active
- `500` `storage_error` — the WAV could not be removed

The operation is idempotent: if no WAV remains, it still returns `200`. In Auto
mode, a successful deletion schedules the usual short AP shutdown grace period.

## 4. Limits

| Constant | Value | Notes |
|---|---|---|
| `kMaxBookBytes` | 1 048 576 | normalized book, after conversion |
| `kMaxBooks` | 20 | subject to partition capacity |
| `kMaxNameBytes` | 48 | after UTF-8-safe sanitization |
| `kReserveBytes` | 2 097 152 | firmware/recovery reserve |
| `kWriteSlackBytes` | 16 384 | extra LittleFS headroom for WAV writes |
| `kPinDigits` | 4 | generated per session |
| WAV format | PCM mono, 16-bit | 8 kHz / 120 s when space permits; otherwise 16 kHz / 30 s |
| Maximum WAV sizes | 1 920 044 / 960 044 bytes | Includes the 44-byte WAV header |

The web page enforces the same size and name limits before sending, so most
errors are caught client-side. The device still enforces all of them.

## 5. Error body

All non-2xx responses use the same shape:

```json
{ "error": "invalid_format" }
```

The web page maps `error` codes to Spanish messages. Unknown codes show a
generic error and keep the book unsent.

## 6. Open points

- `session_closed` is not emitted by the firmware. Ending the session turns off
  the AP, so the page sees a network error and shows that message instead.
  The client keeps the mapping in case a later firmware returns the code.
- Partition size: `partitions.csv` gives LittleFS 6 MiB, above the 2 MiB reserve.
  It assumes 16 MB flash and is unverified until the board is checked.
- An early error (401, 400, 409, 413, 422 mid-stream, 507) drains the unread
  body first, bounded by 1 MiB, so the phone receives the JSON error code.
- A recording requires the selected WAV size, the 2 MiB reserve, and the
  filesystem write margin. If neither format fits, the device refuses to start
  and asks the user to free storage.
- `POST /api/recording/ack` follows successful browser receipt, not a guarantee
  that the user completed the phone's final save-to-Downloads prompt.

- PIN attempts: decided — no lockout and no attempt counter. The AP is
  short-lived, local and user-initiated, so retries are unlimited.
- Upload progress: the browser uses `XMLHttpRequest.upload` progress events.
  Confirm this works on iOS Safari with the AP.
