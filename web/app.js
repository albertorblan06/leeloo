// Browser glue only: DOM wiring around the tested modules in src/. Excluded from coverage.
import {
  acknowledgeRecording, authenticate, errorMessage, fetchRecording, uploadBook,
} from './src/api.js';
import {
  browserStorage, buildNotesText, forgetKey, groqErrorMessage, isPlausibleKey, loadKey, saveKey, summarize,
  transcribe,
} from './src/groq.js';
import { extractPdf, extractErrorMessage } from './src/pdf.js';
import { buildBookFile } from './src/format.js';
import { bookNameFrom } from './src/name.js';

const $ = (id) => document.getElementById(id);
const steps = ['pin', 'choose', 'file', 'busy', 'review', 'done', 'audio-done'];
const kUnknownAuthor = 'Desconocido';

let book = null; // { title, author, pages, text } from the last conversion
let controller = null;
let audioUrl = null;
let audioBlob = null; // the last received recording, kept for transcription
let notesUrl = null;

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

// A download has no progress events here, so the bar shows an unknown wait.
function progressUnknown() {
  $('progress').removeAttribute('value');
}

function busy(label) {
  $('busy-label').textContent = label;
  $('progress').value = 0; // also restores a determinate bar after progressUnknown()
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
    show('choose');
  } catch (err) {
    showError(errorMessage(err.code));
  }
});

$('choose-book').addEventListener('click', () => {
  clearError();
  show('file');
});

// The device deletes its copy only when told how many bytes arrived and the count matches,
// so a refused or failed confirmation just leaves the audio there for another try.
$('choose-audio').addEventListener('click', async () => {
  clearError();
  busy('Recibiendo la grabación…');
  progressUnknown();
  try {
    const blob = await fetchRecording({ signal: controller.signal });
    if (audioUrl) URL.revokeObjectURL(audioUrl);
    audioUrl = URL.createObjectURL(blob);
    audioBlob = blob;
    clearNotes(); // notes of an earlier recording do not belong to this one
    aiStatus('');
    $('audio-player').src = audioUrl;
    $('audio-download').href = audioUrl;
    let status = 'Grabación recibida. Puedes reproducirla o descargarla.';
    try {
      await acknowledgeRecording(blob.size, { signal: controller.signal });
      status += ' Se ha borrado del dispositivo.';
    } catch (err) {
      status += ` Sigue en el dispositivo: ${errorMessage(err.code)}`;
    }
    $('audio-status').textContent = status;
    show('audio-done');
  } catch (err) {
    showError(errorMessage(err.code));
    show(err.code === 'unauthorized' || err.code === 'session_closed' ? 'pin' : 'choose');
  }
});

$('audio-again').addEventListener('click', () => show('choose'));

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

// --- Transcription and summary (Groq, from the phone) ---

// The storage is asked for once and may be absent: a blocked localStorage must not break the page.
const storage = browserStorage();
$('groq-key').value = loadKey(storage);

$('forget-key').addEventListener('click', () => {
  forgetKey(storage);
  $('groq-key').value = '';
  clearError();
  aiStatus('');
});

// The result of Transcribir y resumir is told right under its button, and scrolled into view: the
// page's own error box is at the top, out of sight when the button is far down on a phone.
function aiStatus(text, isError = false) {
  const status = $('ai-status');
  status.textContent = text;
  status.hidden = !text;
  status.classList.toggle('error', isError);
  if (text) status.scrollIntoView({ block: 'center', behavior: 'smooth' });
}

function clearNotes() {
  $('notes').hidden = true;
  $('summary').value = '';
  $('transcript').value = '';
  if (notesUrl) URL.revokeObjectURL(notesUrl);
  notesUrl = null;
}

function showNotes(summary, transcript) {
  $('summary').value = summary;
  $('transcript').value = transcript;
  if (notesUrl) URL.revokeObjectURL(notesUrl);
  notesUrl = URL.createObjectURL(new Blob([buildNotesText({ summary, transcript })], { type: 'text/plain;charset=utf-8' }));
  $('notes-download').href = notesUrl;
  $('summary-block').hidden = !summary; // a failed summary leaves just the transcript
  $('notes').hidden = false;
}

// The text transcription survives a failed summary: it is shown with the reason.
$('transcribe').addEventListener('click', async () => {
  clearError();
  aiStatus('');
  const key = $('groq-key').value.trim();
  if (!audioBlob) {
    aiStatus('Primero recibe la grabación.', true);
    return;
  }
  if (!isPlausibleKey(key)) {
    aiStatus(groqErrorMessage('no_key'), true); // nothing to wait for: say it here, at once
    return;
  }
  busy('Transcribiendo…');
  progressUnknown();
  let transcript;
  try {
    transcript = await transcribe(audioBlob, key, { signal: controller.signal });
  } catch (err) {
    show('audio-done');
    aiStatus(groqErrorMessage(err.code), true);
    return;
  }
  saveKey(storage, key); // Groq accepted it; remembering it must never cost the transcript
  $('busy-label').textContent = 'Resumiendo…';
  let summary = '';
  let summaryError = '';
  try {
    summary = await summarize(transcript, key, { signal: controller.signal });
  } catch (err) {
    summaryError = groqErrorMessage(err.code);
  }
  showNotes(summary, transcript);
  show('audio-done');
  aiStatus(summaryError, true); // empty when the summary worked: the status then hides
  if (!summaryError) $('notes').scrollIntoView({ block: 'start', behavior: 'smooth' });
});

// The page is plain http (the device's own network), where navigator.clipboard is not offered,
// so the text is selected and copied the old way.
$('copy-notes').addEventListener('click', async () => {
  const text = buildNotesText({ summary: $('summary').value, transcript: $('transcript').value });
  let copied = false;
  try {
    await navigator.clipboard.writeText(text);
    copied = true;
  } catch {
    const field = document.createElement('textarea');
    field.value = text;
    document.body.append(field);
    field.focus();
    field.select();
    field.setSelectionRange(0, text.length); // iOS ignores select() alone
    copied = document.execCommand('copy');
    field.remove();
  }
  // Say what happened: on a phone the copy is otherwise invisible.
  const button = $('copy-notes');
  button.textContent = copied ? 'Copiado' : 'No se pudo copiar: selecciona el texto';
  setTimeout(() => { button.textContent = 'Copiar'; }, 2000);
});
