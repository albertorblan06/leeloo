// Browser glue only: DOM wiring around the tested modules in src/. Excluded from coverage.
import { authenticate, uploadBook, errorMessage } from './src/api.js';
import { extractPdf, extractErrorMessage } from './src/pdf.js';
import { buildBookFile } from './src/format.js';
import { bookNameFrom } from './src/name.js';

const $ = (id) => document.getElementById(id);
const steps = ['pin', 'file', 'busy', 'review', 'done'];
const kUnknownAuthor = 'Desconocido';

let book = null; // { title, author, pages, text } from the last conversion
let controller = null;

function show(step) {
  for (const s of steps) $(s).hidden = s !== step;
}

function showError(message) {
  $('error').textContent = message;
  $('error').hidden = false;
}

function clearError() {
  $('error').hidden = true;
}

function busy(label) {
  $('busy-label').textContent = label;
  $('progress').value = 0;
  controller = new AbortController();
  show('busy');
}

let pdfjsPromise;
function loadPdfjs() {
  pdfjsPromise ??= import('./vendor/pdf.mjs')
    .then((m) => {
      m.GlobalWorkerOptions.workerSrc = new URL('./vendor/pdf.worker.mjs', import.meta.url).href;
      return m;
    })
    .catch((err) => {
      pdfjsPromise = undefined; // allow a retry instead of caching the failure
      throw err;
    });
  return pdfjsPromise;
}

$('pin').addEventListener('submit', async (e) => {
  e.preventDefault();
  clearError();
  try {
    await authenticate($('pin-input').value.trim());
    show('file');
  } catch (err) {
    showError(errorMessage(err.code));
  }
});

$('file-input').addEventListener('change', async (e) => {
  const file = e.target.files[0];
  e.target.value = ''; // lets the same file be picked again after an error
  if (!file) return;
  clearError();
  busy('Convirtiendo…');
  try {
    const pdfjs = await loadPdfjs();
    const r = await extractPdf(pdfjs, await file.arrayBuffer(), {
      onProgress: (p) => { $('progress').value = p; },
      signal: controller.signal,
    });
    book = { ...r, title: r.title || file.name.replace(/\.pdf$/i, '') };
    $('title').value = book.title;
    $('author').value = book.author;
    $('stats').textContent = `${r.pages} páginas, ${r.text.split(' ').length} palabras.`;
    show('review');
  } catch (err) {
    showError(extractErrorMessage(err.code));
    show('file');
  }
});

$('review').addEventListener('submit', async (e) => {
  e.preventDefault();
  clearError();
  let name;
  let file;
  try {
    name = bookNameFrom($('title').value);
    file = buildBookFile({
      title: $('title').value,
      author: $('author').value.trim() || kUnknownAuthor,
      pages: book.pages,
      body: book.text,
    });
  } catch (err) {
    // bookNameFrom fails on an empty title; buildBookFile then only fails on size.
    showError(name === undefined ? 'Escribe un título.' : errorMessage('too_large'));
    return;
  }
  busy('Enviando…');
  try {
    await uploadBook(name, file, {
      onProgress: (p) => { $('progress').value = p; },
      signal: controller.signal,
    });
    show('done');
  } catch (err) {
    showError(errorMessage(err.code));
    show(err.code === 'unauthorized' || err.code === 'session_closed' ? 'pin' : 'review');
  }
});

$('cancel').addEventListener('click', () => controller?.abort());
$('again').addEventListener('click', () => show('file'));
