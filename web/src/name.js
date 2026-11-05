import { utf8ByteLength } from './format.js';
import { kMaxNameBytes } from './limits.js';

// Device-side name: no separators or control characters, at most kMaxNameBytes of UTF-8.
// The device sanitizes again; this only avoids sending a name it would reject.
export function bookNameFrom(title) {
  const clean = title.replace(/[\u0000-\u001f\u007f/\\:*?"<>|]/g, '').trim();
  let name = '';
  for (const ch of clean) { // iterates code points, so surrogate pairs stay whole
    if (utf8ByteLength(name + ch) > kMaxNameBytes) break;
    name += ch;
  }
  name = name.trim();
  if (name === '') {
    throw new RangeError('book name is empty after sanitizing');
  }
  return name;
}
