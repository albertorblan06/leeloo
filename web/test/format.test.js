import { describe, it, expect } from 'vitest';
import { buildBookFile, utf8ByteLength } from '../src/format.js';
import { kMaxBookBytes, kMaxHeaderValueBytes } from '../src/limits.js';

describe('utf8ByteLength', () => {
  it('counts ASCII as one byte per character', () => {
    expect(utf8ByteLength('abc')).toBe(3);
  });

  it('counts Spanish accented characters as two bytes', () => {
    expect(utf8ByteLength('ñá')).toBe(4);
  });

  it('returns zero for an empty string', () => {
    expect(utf8ByteLength('')).toBe(0);
  });
});

describe('buildBookFile', () => {
  it('writes title, author, pages, delimiter and body in order', () => {
    const file = buildBookFile({ title: 'Título', author: 'Autor', pages: 120, body: 'hola mundo' });
    expect(file).toBe('TITLE: Título\nAUTHOR: Autor\nPAGES: 120\n###START###\nhola mundo');
  });

  it('omits the PAGES line when pages is unknown', () => {
    const file = buildBookFile({ title: 'T', author: 'A', body: 'x' });
    expect(file).toBe('TITLE: T\nAUTHOR: A\n###START###\nx');
  });

  it('uses LF line endings only', () => {
    const file = buildBookFile({ title: 'T', author: 'A', pages: 1, body: 'x' });
    expect(file).not.toContain('\r');
  });

  it('replaces line breaks inside header values so the header stays one line each', () => {
    const file = buildBookFile({ title: 'Línea\nuno\r\ndos', author: 'A', body: 'x' });
    expect(file.split('\n')[0]).toBe('TITLE: Línea uno dos');
  });

  it('caps long header values so the device header line stays within its limit', () => {
    const file = buildBookFile({ title: 'ñ'.repeat(300), author: 'A', body: 'x' });
    const titleLine = file.split('\n')[0];
    expect(utf8ByteLength(titleLine)).toBeLessThanOrEqual(kMaxHeaderValueBytes + 7);
  });

  it('truncates a long header value without splitting a code point', () => {
    const file = buildBookFile({ title: 'ñ'.repeat(300), author: 'A', body: 'x' });
    expect(file.split('\n')[0]).toBe(`TITLE: ${'ñ'.repeat(kMaxHeaderValueBytes / 2)}`);
  });

  it('rejects an empty title after trimming', () => {
    expect(() => buildBookFile({ title: '   ', author: 'A', body: 'x' })).toThrow(RangeError);
  });

  it('rejects an empty body', () => {
    expect(() => buildBookFile({ title: 'T', author: 'A', body: '' })).toThrow(RangeError);
  });

  it('rejects a book whose UTF-8 size exceeds the device limit', () => {
    const body = 'a'.repeat(kMaxBookBytes);
    expect(() => buildBookFile({ title: 'T', author: 'A', body })).toThrow(RangeError);
  });

  it('accepts a book exactly at the device limit', () => {
    const headerless = buildBookFile({ title: 'T', author: 'A', body: 'x' });
    const room = kMaxBookBytes - utf8ByteLength(headerless) + 1;
    const body = 'a'.repeat(room);
    expect(utf8ByteLength(buildBookFile({ title: 'T', author: 'A', body }))).toBe(kMaxBookBytes);
  });

  it('counts multibyte characters toward the limit by bytes, not characters', () => {
    const body = 'ñ'.repeat(Math.ceil(kMaxBookBytes / 2));
    expect(() => buildBookFile({ title: 'T', author: 'A', body })).toThrow(RangeError);
  });
});
