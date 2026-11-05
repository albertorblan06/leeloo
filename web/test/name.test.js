import { describe, it, expect } from 'vitest';
import { bookNameFrom } from '../src/name.js';
import { utf8ByteLength } from '../src/format.js';
import { kMaxNameBytes } from '../src/limits.js';

describe('bookNameFrom', () => {
  it('keeps a plain Spanish title', () => {
    expect(bookNameFrom('Don Quijote ñ')).toBe('Don Quijote ñ');
  });

  it('strips path separators and control characters', () => {
    expect(bookNameFrom('a/b\\c:d\u0007e')).toBe('abcde');
  });

  it('trims surrounding whitespace', () => {
    expect(bookNameFrom('  hola  ')).toBe('hola');
  });

  it('truncates to the byte limit without splitting a code point', () => {
    const name = bookNameFrom('ñ'.repeat(100));
    expect(utf8ByteLength(name)).toBeLessThanOrEqual(kMaxNameBytes);
    expect(name).toBe('ñ'.repeat(kMaxNameBytes / 2));
  });

  it('does not split a surrogate pair at the limit', () => {
    const name = bookNameFrom('a'.repeat(kMaxNameBytes - 2) + '😀');
    expect(name).toBe('a'.repeat(kMaxNameBytes - 2));
  });

  it('throws when nothing is left after sanitizing', () => {
    expect(() => bookNameFrom(' /\\: ')).toThrow(RangeError);
  });

  it('throws for an empty title', () => {
    expect(() => bookNameFrom('')).toThrow(RangeError);
  });
});
