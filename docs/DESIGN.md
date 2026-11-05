# Leeloo Design Guide

Status: draft, 2026-11-05. Covers the OLED menu and the phone web app.
Source of truth for the web tokens: Loudop (loudop.app), measured on 2026-11-05 with a guest account.

## 1. Principles

These carry over from Loudop. Colors do not.

1. **Calm and sparse.** Loudop uses one background, one text color, one accent, and thin rules. Leeloo follows: few elements per screen, no decoration.
2. **Hierarchy by weight and size, not color.** Titles are larger and serif on Loudop. On the OLED the same role is played by size and inversion.
3. **Quiet feedback.** Selection and focus are obvious but not loud. The accent (red `#b3382c`) marks focus only.
4. **Short Spanish copy.** Infinitive or imperative verbs, one or two words: "Seguir", "Omitir", "Vincular con Google". Sentence case.
5. **Information first.** The reading screen shows WPM and battery only (ARCHITECTURE §3.2). Menus show only what the user needs to choose.

## 2. Web app tokens (measured on Loudop)

Light theme, from `:root` custom properties:

| Token | Value | Use |
|---|---|---|
| `--bg` | `#f2f0eb` | Page background |
| `--surface` | `#eae7e1` | Raised or grouped surfaces |
| `--text` | `#3b3935` | Primary text |
| `--muted` | `#6b675f` | Secondary text, labels, inactive tabs |
| `--line` | `#ddd9d1` | Row dividers, 1px |
| `--focus` | `#b3382c` | Focus ring, accent |
| `--serif` | `Charter, "Iowan Old Style", "Palatino Linotype", Georgia, serif` | Titles |
| `--sans` | `system-ui, -apple-system, "Segoe UI", sans-serif` | Body, controls |
| `--column` | `34rem` | Max content width |

Dark theme, measured from Loudop's stylesheet (same token names, only these change):

| Token | Light | Dark |
|---|---|---|
| `--bg` | `#f2f0eb` | `#1f1e1c` |
| `--surface` | `#eae7e1` | `#282725` |
| `--text` | `#3b3935` | `#cfcbc3` |
| `--muted` | `#6b675f` | `#96928a` |
| `--line` | `#ddd9d1` | `#34322f` |
| `--focus` | `#b3382c` | `#e0766a` |

### Theme switching (web app)

The phone web page supports light and dark, with the same mechanism as Loudop:

- Default follows the phone: dark values apply under `@media (prefers-color-scheme: dark)` on `:root:not([data-theme="light"])`.
- The user can force a theme with `data-theme="light"` or `data-theme="dark"` on `<html>`. The selector `:root[data-theme="dark"]` carries the same dark values.
- The control is the three-way segmented control "Sistema · Claro · Oscuro" (see components below), default "Sistema".
- The choice is stored in the browser on the phone (`localStorage`), not on the device. The device keeps no theme setting (§3).
- Every color comes from the tokens above. No hardcoded colors in the page.

Motion tokens: `--t-fast .11s`, `--t-base .2s`, `--t-slow .34s`, `--ease cubic-bezier(.22, 1, .36, 1)`, `--ease-spring cubic-bezier(.34, 1.56, .64, 1)`, press scale `.97`, stagger `40ms`. Web app only; no animation on the OLED.

### Type scale (measured)

| Role | Font | Size / line-height | Weight |
|---|---|---|---|
| Hero title ("Buenos días") | serif | 30px / 36px | 400 |
| Welcome title ("Aprende.") | serif | 28px / 35px | 600 |
| Body | sans | 15px / normal | 400 |
| Control text ("Seguir", "Omitir") | sans | 13px, letter-spacing .26px | 300 |
| Section label, app mark | sans | 12.5px, letter-spacing 1px | 400 |

Section labels (for example "APARIENCIA", "AVISOS") are small uppercase, muted. Their exact size was estimated from the screenshot, not measured.

### Layout and components (from settings screenshots, 390px wide)

- Side gutter: 16px.
- List rows: divided by 1px `--line`, about 51px tall, label left, value or chevron right.
- Value on the right: current setting ("Sistema", "Sí"). A chevron `>` means it opens a sub-screen.
- Segmented control for three-way choices ("Sistema · Claro · Oscuro"): selected item has a 1px underline in `--text`; unselected are `--muted`.
- Outlined buttons for primary account actions ("Vincular con Google"): full width, rounded corners (about 8px, estimated), 1px border, no fill.
- Bottom tab bar with labels and page dots. Active label in `--text`, inactive in `--muted`.
- Back link at top: chevron + parent name ("< Perfil").
- Corner radius on the page itself is 0. Shadows: none.

## 3. OLED rules (128x64, monochrome)

The display is 1-bit. Anything that looks like a "color" on the web becomes black, white, or inversion.

**The device is always dark.** Background is unlit pixels, content is lit pixels. There is no light theme, no theme setting, and no "Modo" row in Settings. Loudop's light/dark choice does not exist on the device. Only brightness is configurable (Auto or Manual, ARCHITECTURE §3.2).

### 3.1 Grid

- Screen: 128 x 64 px. Side margin 2 px. Usable width 124 px.
- Status bar: y 0–9, followed by a 1 px rule at y 11.
- List area: y 13–63. Holds 4 rows of 12 px (y 14, 26, 38, 50) plus a scroll gutter 3 px wide at x 124–127 when the list is longer than 4 items.
- Font: a bitmap font with the Spanish glyph set (á é í ó ú ñ ü Ñ ¿ ¡). Use a 6 px wide, 10 px tall face for body text and labels. Confirm the chosen u8g2 font contains every glyph in §7 before use; if it does not, fall back to a font that does and record it here.

### 3.2 Hierarchy

| Role | Rendering |
|---|---|
| Status (WPM, battery %) | Top bar, 6x10, left and right aligned |
| Section title | Uppercase, 6x10, one line above its list |
| Item | 6x10, left aligned, 2 px left padding |
| Focused item | Inverted: white bar, black text, full list width |
| Value | Right-aligned in the row, 6x10 |
| Submenu marker | `>` as the last character of the row |
| Toggle | Text `Sí` / `No`, not a graphic switch |
| Cycle choice | `< Valor >` (for example `< Auto >`) |

The focused row is the OLED equivalent of Loudop's `--focus`. Do not draw an extra border around it.

### 3.3 Scrolling and wrapping

- Rotation moves the cursor and wraps around at both ends (`lib/menu/menu.h`).
- The window scrolls so the cursor stays visible. Keep at least one item above or below the cursor when possible.
- Text longer than the row is truncated with `…`. Never wrap a menu item onto two lines.

### 3.4 Motion

No animation. The screen changes instantly. Animation costs I2C time that belongs to RSVP timing (ARCHITECTURE §4.2, I2C bus owner).

## 4. Main menu

Proposed items, in order. Confirm with the user (see §9).

```
 WPM 300              87%      <- status bar (y 0-9)
-----------------------------   <- rule (y 11)
 Continuar                      <- focused (inverted)
 Biblioteca                     >
 Subir libro                    >
 Ajustes                        >
```

- Rotation: move cursor. Click: open the item.
- Long press: enter deep sleep (ARCHITECTURE §4.3). Show a one-line confirmation only if the wake gesture needs explaining; decide at hardware gate.
- "Continuar" is shown only when a progress record exists. Without it the list starts at "Biblioteca".

## 5. Other screens (shared rules)

Each screen follows §3. Specific layouts are defined when each state is built; the rules below apply to all of them.

| Screen | Rule |
|---|---|
| Boot | Only the text `Leeloo`, centered, 12 px high. No progress bar. |
| Library | Title row, then list of books. Truncate titles. Empty state: `Sin libros` plus one line of help. |
| Reading | Only WPM and battery %. The word is the largest text on screen. |
| Paused | Progress and time may share the screen with controls, as allowed by ARCHITECTURE §3.2. Use one section per screen; rotation pages between them. |
| Settings | Rows with values on the right (§3.2). Toggles as `Sí` / `No`. |
| Wi-Fi Upload | Show SSID, PIN, and address on three lines. Long press exits (ARCHITECTURE §4.3). |
| Error | One line title in uppercase, up to two lines of help in Spanish, then `Pulsa para volver`. |

Settings mapping from Loudop's Preferencias screen:

| Loudop | Leeloo OLED |
|---|---|
| Modo: Sistema / Claro / Oscuro | Not ported: the device is always dark. Nearest setting is Brillo: `< Auto >` / `< Manual >` (ARCHITECTURE §3.2) |
| Avisos: Sí / No per item | Háptico: `Sí` / `No` |
| Sub-screens with chevron | Submenu with `>` |

## 6. Input conventions

From ARCHITECTURE §4.3:

- Rotate: move in menus, change WPM while reading, seek while paused.
- Click: select in menus, pause while reading, resume while paused.
- Long press: back to main menu from reading or paused; exit Wi-Fi; sleep from main menu.

Haptics on select: optional short pulse, off by default until measured on hardware (`haptic service`, ARCHITECTURE §4.2).

## 7. Copy and Spanish strings

- Sentence case. Verbs in the infinitive for actions ("Subir libro"), nouns for destinations ("Biblioteca", "Ajustes").
- No punctuation at the end of menu labels. Help text ends with a period.
- Use `¿` and `¡` only in full sentences in help text and error text.
- Glyph set the font must cover: `á é í ó ú ü ñ Á É Í Ó Ú Ü Ñ ¿ ¡`, plus `…` for truncation.
- Tone: short, calm, not playful. Loudop uses "Has terminado por hoy." as a closing line; Leeloo may use the same tone for "Fin del libro".

## 8. Accessibility

- Contrast on the OLED is maximum (lit on unlit, always dark). Do not use gray text or dithering for secondary text; dimming is handled by brightness, not by the design.
- Minimum text size: 6x10 px. Do not go smaller.
- Focus is always visible (inversion). Never rely on a color cue.
- Every control reachable with rotation and click only.
- On the web app, keep the Loudop focus ring (`--focus`) on all interactive elements and maintain 16px gutters.

## 9. Open questions

1. **Main menu items.** Confirm: Continuar, Biblioteca, Subir libro, Ajustes. Should Deep sleep be a menu item or long press only (current spec)?
2. **Font.** Pick the u8g2 bitmap font after checking the Spanish glyph set against §7.
3. **Web app.** Should the phone web app use Loudop's serif for titles, or sans only? Serif is measured; Charter is not bundled on all phones.
4. **Web theme storage.** `localStorage` on the phone is assumed for the Claro/Oscuro choice. The page is served from the device's temporary AP, so its origin changes between sessions and the choice may reset; if that is unacceptable, the fallback is following the system theme only.
5. **Haptic on select.** Decide on hardware.
6. **Exact section-label size and button radius.** Measure from the DOM; currently estimated.

## 10. Source notes

- Captured: `/welcome`, `/` (home), `/profile`, `/profile/preferencias`, `/profile/cuenta`, `/profile/temas`, `/friends`, `/biblioteca`, all on loudop.app, guest session, 390px phone width unless noted.
- Not captured: Comunidad (`/community`) and the lesson flow (`/today/ia`).
- Dark mode: all six color tokens read from Loudop's stylesheet (light and dark computed values verified on `/profile/preferencias`).
