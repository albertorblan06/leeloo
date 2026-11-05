# RSVP reading on hardware

Status: **written, not yet run on hardware.** Covers the Leer playback: `lib/reader` (pure, tested on host) wired by `src/main.cpp` and drawn by `src/display_ui.cpp`. Behavior is specified in `docs/DEVICE-MENU.md` §2 and §6.

## Before flashing
- [ ] `pio run -e esp32s3` builds without warnings.
- [ ] PSRAM is enabled for the board in use. Without it, opening any book shows `No se pudo abrir el libro`. Record the PSRAM size reported at boot: ______ MB.

## Book opening
- [ ] Upload a PDF-converted book of about 1 MiB from the phone (WEB-API §3.2). Open it from Leer. The first word appears within about one second.
- [ ] A book with a damaged header (no `###START###` line) shows `No se pudo abrir` and returns to the list.
- [ ] Opening a book that is only whitespace after the header goes straight to `LIBRO TERMINADO`.

## Playback
- [ ] The word is large (18 pt) and centered at about the middle of the screen. Its focal letter is hollow (1 px white contour, black inside) and sits on the center column, between the two short vertical marks.
- [ ] Words wider than the screen split at a syllable boundary with a trailing `-`. Each piece is one beat; the last piece holds the word's punctuation. Note any split that reads badly: ______.
- [ ] Words with accents and ñ render correctly: ¿Qué, pasó, niño, ¡Hola. Missing glyphs go into DEVICE-MENU §6.
- [ ] Words advance at the shown WPM (default 300, about 200 ms per word). Time 100 words with a stopwatch: about 20 s.
- [ ] A comma or sentence end visibly pauses longer than an ordinary word.
- [ ] A word wider than 18 characters shows `...` at the end and does not overflow the screen.
- [ ] While reading, UP raises the WPM by 25 and DOWN lowers it by 25, within 150–800. The label `N WPM` appears in the top-left corner and disappears after about 2 s.
- [ ] Nothing else appears on screen during playback: no progress bar, no extra text.

## Pause and sentence navigation
- [ ] SELECT short pauses. The top-left shows `PAUSA` and the word shown is the first word of the current piece (not a split half).
- [ ] While paused, UP goes to the first word of the previous sentence and DOWN to the first word of the next sentence. Both stop at the book's ends.
- [ ] Going back from the middle of a sentence returns to that sentence's first word, not to the previous sentence.
- [ ] A paragraph break without a period ends a sentence.
- [ ] SELECT short resumes from the word shown. A split word restarts from its first piece.
- [ ] SELECT long returns to the book list from both playing and paused states.

## Resume and end
- [ ] Pause mid-book, leave with SELECT long, reopen the book: it resumes at the same word.
- [ ] Power-cycle while reading (without pausing): the book reopens at the last saved pause or exit point, not at the power-off point. Record the difference: ______.
- [ ] Reading to the last word shows `LIBRO TERMINADO` with `Releer` and `Volver al menú`.
- [ ] `Releer` starts at the first word. `Volver al menú` returns to the book list.
- [ ] After finishing a book, its saved position is 0 (`<título>.pos` holds `0`): reopening it starts at the beginning.
- [ ] Deleting a book from Biblioteca > Borrar removes its position as well. Reupload it and open it: it starts at the beginning.

## Storage
- [ ] The book list does not show `.pos` files as books.
- [ ] Remove the power during a pause save. After boot, the book still opens (at the old or the new position, never a crash or a blank screen).
