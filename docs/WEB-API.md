# Upload web API contract

Contract between the phone web app (`web/`) and the ESP32 firmware for the
temporary Upload Book session. Source of truth for endpoint shapes, error codes
and limits. Firmware constants must match section 4.

Transport: plain HTTP over the device soft AP (no TLS, no internet, no external
hosts). The page never loads anything from outside the AP.

## 1. Session lifecycle

- The device starts the soft AP and generates a new 4-digit PIN only when the
  user selects Upload Book.
- The PIN is shown on the OLED. The web page must send it before any upload.
- A successful PIN check opens the session for that client.
- The session ends when the user exits upload mode, long-presses to cancel, or
  the AP shuts down. After that every request returns `session_closed`.
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

Measured size: 548 817 bytes gzip in total (pdf.js is 541 814 of those). No CDN,
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

A book whose sanitized name already exists replaces it. The old file is moved
aside, the new one is renamed into place, and only then is the old one deleted;
if the rename fails the old book is restored. It does not count toward
`library_full`.

Firmware writes the body to a temporary file, validates it, then renames it into
place (atomic). Incomplete temporary files are removed at boot.

## 4. Limits

| Constant | Value | Notes |
|---|---|---|
| `kMaxBookBytes` | 1 048 576 | normalized book, after conversion |
| `kMaxBooks` | 20 | subject to partition capacity |
| `kMaxNameBytes` | 48 | after UTF-8-safe sanitization |
| `kReserveBytes` | 2 097 152 | firmware/recovery reserve |
| `kPinDigits` | 4 | generated per session |

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

- PIN attempts: decided — no lockout and no attempt counter. The AP is
  short-lived, local and user-initiated, so retries are unlimited.
- Upload progress: the browser uses `XMLHttpRequest.upload` progress events.
  Confirm this works on iOS Safari with the AP.
