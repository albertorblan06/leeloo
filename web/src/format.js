import { kMaxBookBytes, kMaxHeaderValueBytes } from './limits.js';

const encoder = new TextEncoder();

export function utf8ByteLength(text) {
  return encoder.encode(text).length;
}

// Header values must stay on one line, so embedded line breaks become spaces.
// Long values (PDF metadata can be huge) are cut at a code-point boundary.
function headerValue(value, field) {
  const clean = value.replace(/[\r\n]+/g, ' ').trim();
  if (clean === '') {
    throw new RangeError(`${field} must not be empty`);
  }
  let out = '';
  for (const ch of clean) {
    if (utf8ByteLength(out + ch) > kMaxHeaderValueBytes) break;
    out += ch;
  }
  return out.trim();
}

// Builds the single-file format from docs/ARCHITECTURE.md §5.1. Throws RangeError
// on invalid input so a bad book never reaches the device.
export function buildBookFile({ title, author, pages, body }) {
  if (body === '') {
    throw new RangeError('body must not be empty');
  }
  const header = [
    `TITLE: ${headerValue(title, 'title')}`,
    `AUTHOR: ${headerValue(author, 'author')}`,
  ];
  if (pages !== undefined) {
    header.push(`PAGES: ${pages}`);
  }
  const file = `${header.join('\n')}\n###START###\n${body}`;
  if (utf8ByteLength(file) > kMaxBookBytes) {
    throw new RangeError('book exceeds the device size limit');
  }
  return file;
}
