import { describe, it, expect } from 'vitest';
import { normalizeText } from '../src/normalize.js';

describe('normalizeText', () => {
  it('returns an empty string for empty input', () => {
    expect(normalizeText('')).toBe('');
  });

  it('returns an empty string for whitespace-only input', () => {
    expect(normalizeText(' \n\t \r\n ')).toBe('');
  });

  it('collapses runs of whitespace into single spaces', () => {
    expect(normalizeText('uno   dos\n\ntres\t\tcuatro')).toBe('uno dos tres cuatro');
  });

  it('keeps Spanish multibyte characters intact', () => {
    expect(normalizeText('¿Qué año es? ¡Ñandú!')).toBe('¿Qué año es? ¡Ñandú!');
  });

  it('composes decomposed accents to NFC', () => {
    const decomposed = 'café';
    expect(normalizeText(decomposed)).toBe('café');
  });

  it('joins a word hyphenated across a line break', () => {
    expect(normalizeText('la re-\nlación es')).toBe('la relación es');
  });

  it('keeps a hyphen when the next line starts with an uppercase letter', () => {
    expect(normalizeText('Madrid-\nBarcelona')).toBe('Madrid- Barcelona');
  });

  it('keeps a standalone dash surrounded by spaces', () => {
    expect(normalizeText('uno - dos')).toBe('uno - dos');
  });

  it('removes lines that contain only a page number', () => {
    expect(normalizeText('fin del capítulo\n  42  \nnuevo texto')).toBe('fin del capítulo nuevo texto');
  });

  it('keeps numbers that share a line with text', () => {
    expect(normalizeText('en 1984 pasó')).toBe('en 1984 pasó');
  });

  it('handles a large input without losing content', () => {
    const word = 'palabra ';
    const big = word.repeat(100_000);
    expect(normalizeText(big)).toBe(big.trim());
  });
});
