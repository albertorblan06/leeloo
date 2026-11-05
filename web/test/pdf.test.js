import { describe, it, expect, vi } from 'vitest';
import { ExtractError, extractPdf, extractErrorMessage } from '../src/pdf.js';

// Minimal pdf.js double: pages is an array of arrays of {str, hasEOL}.
function fakePdfjs({ pages, info = {}, failWith } = {}) {
  const destroy = vi.fn();
  return {
    destroy,
    getDocument: () => ({
      promise: failWith
        ? Promise.reject(failWith)
        : Promise.resolve({
          numPages: pages.length,
          getMetadata: async () => ({ info }),
          getPage: async (n) => ({
            getTextContent: async () => ({ items: pages[n - 1] }),
            cleanup: () => {},
          }),
        }),
      destroy,
    }),
  };
}

const words = (s) => s.split(' ').map((str) => ({ str, hasEOL: false }));
const longText = 'Había una vez un molino junto al río donde vivía un molinero con su hija';

describe('extractPdf', () => {
  it('returns normalized text, page count and PDF metadata', async () => {
    const pdfjs = fakePdfjs({ pages: [words(longText), words('segunda página')], info: { Title: 'El molino', Author: 'Ana' } });
    const r = await extractPdf(pdfjs, new ArrayBuffer(1));
    expect(r.title).toBe('El molino');
    expect(r.author).toBe('Ana');
    expect(r.pages).toBe(2);
    expect(r.text).toBe(`${longText} segunda página`);
  });

  it('returns empty strings for missing metadata', async () => {
    const r = await extractPdf(fakePdfjs({ pages: [words(longText)] }), new ArrayBuffer(1));
    expect(r.title).toBe('');
    expect(r.author).toBe('');
  });

  it('rejoins words hyphenated across a line end', async () => {
    const page = [{ str: 'la re-', hasEOL: true }, { str: 'lación entre el molino y el río de la aldea vieja', hasEOL: false }];
    const r = await extractPdf(fakePdfjs({ pages: [page] }), new ArrayBuffer(1));
    expect(r.text.startsWith('la relación entre')).toBe(true);
  });

  it('reports progress per page as a 0..1 fraction', async () => {
    const onProgress = vi.fn();
    await extractPdf(fakePdfjs({ pages: [words(longText), words(longText)] }), new ArrayBuffer(1), { onProgress });
    expect(onProgress.mock.calls.map((c) => c[0])).toEqual([0.5, 1]);
  });

  it('throws no_text for a PDF without selectable text', async () => {
    await expect(extractPdf(fakePdfjs({ pages: [[], []] }), new ArrayBuffer(1))).rejects.toMatchObject({ code: 'no_text' });
  });

  it('throws encrypted for a password-protected PDF', async () => {
    const pdfjs = fakePdfjs({ failWith: Object.assign(new Error('pw'), { name: 'PasswordException' }) });
    await expect(extractPdf(pdfjs, new ArrayBuffer(1))).rejects.toMatchObject({ code: 'encrypted' });
  });

  it('throws unreadable for a corrupt PDF', async () => {
    const pdfjs = fakePdfjs({ failWith: Object.assign(new Error('bad'), { name: 'InvalidPDFException' }) });
    await expect(extractPdf(pdfjs, new ArrayBuffer(1))).rejects.toMatchObject({ code: 'unreadable' });
  });

  it('stops with aborted and destroys the loading task when the signal fires', async () => {
    const ctrl = new AbortController();
    const pdfjs = fakePdfjs({ pages: [words(longText), words(longText)] });
    const onProgress = vi.fn(() => ctrl.abort());
    await expect(extractPdf(pdfjs, new ArrayBuffer(1), { onProgress, signal: ctrl.signal }))
      .rejects.toMatchObject({ code: 'aborted' });
    expect(pdfjs.destroy).toHaveBeenCalled();
  });

  it('rejects immediately when the signal is already aborted', async () => {
    const ctrl = new AbortController();
    ctrl.abort();
    await expect(extractPdf(fakePdfjs({ pages: [words(longText)] }), new ArrayBuffer(1), { signal: ctrl.signal }))
      .rejects.toBeInstanceOf(ExtractError);
  });
});

describe('extractErrorMessage', () => {
  it('has a Spanish message for each extraction error', () => {
    for (const code of ['no_text', 'encrypted', 'unreadable', 'aborted']) {
      expect(extractErrorMessage(code)).not.toMatch(/desconocido/i);
    }
  });

  it('falls back to a generic message', () => {
    expect(extractErrorMessage('x')).toMatch(/desconocido/i);
  });
});
