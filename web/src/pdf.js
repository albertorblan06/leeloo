import { normalizeText } from './normalize.js';

export class ExtractError extends Error {
  constructor(code) {
    super(code);
    this.code = code;
  }
}

const messages = {
  no_text: 'Este PDF no tiene texto seleccionable (¿es un escaneo?). El OCR no está soportado.',
  encrypted: 'Este PDF está protegido con contraseña y no se puede leer.',
  unreadable: 'No se pudo leer el PDF. Puede estar dañado.',
  aborted: 'Conversión cancelada.',
};

export function extractErrorMessage(code) {
  return messages[code] ?? 'Error desconocido al convertir el PDF.';
}

// Below this many characters the PDF is treated as having no real text (e.g. a scan
// with a stray page number). Initial value, not tuned on real PDFs.
const kMinTextChars = 20;

// `pdfjs` is injected so this module stays testable in Node; the browser passes the
// vendored pdf.js module.
export async function extractPdf(pdfjs, data, { onProgress, signal } = {}) {
  if (signal?.aborted) {
    throw new ExtractError('aborted');
  }
  const task = pdfjs.getDocument({ data, isEvalSupported: false });
  signal?.addEventListener('abort', () => task.destroy(), { once: true });
  try {
    const doc = await task.promise;
    const { info = {} } = await doc.getMetadata();
    const parts = [];
    for (let n = 1; n <= doc.numPages; n++) {
      const page = await doc.getPage(n);
      const { items } = await page.getTextContent();
      parts.push(items.map((it) => it.str + (it.hasEOL ? '\n' : ' ')).join(''));
      page.cleanup();
      onProgress?.(n / doc.numPages);
      if (signal?.aborted) {
        throw new ExtractError('aborted');
      }
    }
    const text = normalizeText(parts.join('\n'));
    if (text.length < kMinTextChars) {
      throw new ExtractError('no_text');
    }
    return { title: info.Title ?? '', author: info.Author ?? '', pages: doc.numPages, text };
  } catch (err) {
    if (err instanceof ExtractError) {
      throw err;
    }
    if (signal?.aborted) {
      throw new ExtractError('aborted');
    }
    throw new ExtractError(err?.name === 'PasswordException' ? 'encrypted' : 'unreadable');
  } finally {
    task.destroy();
  }
}
