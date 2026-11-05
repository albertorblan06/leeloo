// Turns raw text extracted from a PDF into the single-space word stream the device expects.
export function normalizeText(raw) {
  return raw
    .normalize('NFC')
    .replace(/\r\n?/g, '\n')
    // A word split by a hyphen at a line end is rejoined; a capital after the break
    // means a real compound (e.g. "Madrid-Barcelona"), so the hyphen is kept.
    .replace(/(\p{L})-[ \t]*\n[ \t]*(\p{Ll})/gu, '$1$2')
    // shortcut: drops only bare page-number lines; repeated running headers are kept,
    // upgrade when real PDFs show them hurting RSVP reading.
    .split('\n')
    .filter((line) => !/^\s*\d+\s*$/.test(line))
    .join('\n')
    .replace(/\s+/g, ' ')
    .trim();
}
