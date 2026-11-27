// Transcription and summary of a recording with the Groq API, called straight from the phone.
// The device never sees the key or the text. Model ids and parameters were checked against
// console.groq.com/docs (speech-to-text, models, reasoning) on 2026-11-27: they live here only,
// so a deprecation is a one-line change.
export const kGroqBase = 'https://api.groq.com/openai/v1';
export const kTranscriptionModel = 'whisper-large-v3-turbo';
export const kSummaryModel = 'openai/gpt-oss-20b';
// Free-tier limit for an uploaded file (25 MB; 100 MB on the dev tier).
export const kMaxAudioBytes = 25 * 1024 * 1024;
export const kKeyStorageName = 'leeloo_groq_key';

export class GroqError extends Error {
  constructor(code, status = 0) {
    super(code); // the code only: never the response text, which could echo the key
    this.code = code;
    this.status = status;
  }
}

const messages = {
  no_key: 'Pega tu clave de Groq para transcribir.',
  invalid_key: 'Groq no acepta esa clave. Revísala o crea una nueva en console.groq.com.',
  network: 'No hay internet. Activa los datos móviles y vuelve a probar.',
  too_large: 'La grabación es demasiado grande para Groq (máximo 25 MB).',
  rate_limit: 'Groq está limitando las peticiones. Espera un minuto y vuelve a probar.',
  model_not_found: 'Groq ya no ofrece el modelo que usa Leeloo. Hay que actualizar la página del dispositivo.',
  server: 'Groq no responde ahora. Inténtalo de nuevo en unos minutos.',
  text_too_long: 'La transcripción es demasiado larga para resumirla ahora. El texto sí está disponible.',
  bad_request: 'Groq no pudo procesar la grabación.',
  empty: 'No se ha reconocido ninguna palabra en la grabación.',
  empty_summary: 'Groq no devolvió ningún resumen. La transcripción sí está disponible.',
  aborted: 'Operación cancelada.',
};

export function groqErrorMessage(code) {
  return messages[code] ?? 'Error desconocido de Groq.';
}

// Groq keys start with gsk_ and are printable ASCII. Anything else (a pasted invisible or typographic
// character) would make fetch throw on the header and look like a network failure, so it is
// rejected here. Groq has the last word on whether the key is valid.
export function isPlausibleKey(key) {
  return typeof key === 'string' && /^gsk_[\x21-\x7E]{16,}$/.test(key.trim());
}

function codeFromResponse(status, body) {
  // A retired model is answered with either code, and with 400 as well as 404.
  if (body?.error?.code === 'model_not_found' || body?.error?.code === 'model_decommissioned') {
    return 'model_not_found';
  }
  if (status === 401 || status === 403) return 'invalid_key';
  if (status === 413) return 'too_large';
  if (status === 429) return 'rate_limit';
  if (status >= 500) return 'server';
  return 'bad_request';
}

async function send(url, init, { signal, fetchFn }) {
  let res;
  try {
    res = await fetchFn(url, { ...init, signal });
  } catch {
    throw new GroqError(signal?.aborted ? 'aborted' : 'network');
  }
  if (!res.ok) {
    let body;
    try {
      body = await res.json();
    } catch {
      // A non-JSON error body: the status alone decides.
    }
    throw new GroqError(codeFromResponse(res.status, body), res.status);
  }
  try {
    return await res.json();
  } catch {
    throw new GroqError(signal?.aborted ? 'aborted' : 'server', res.status);
  }
}

export async function transcribe(blob, key, { signal, fetchFn = fetch, language = 'es' } = {}) {
  if (!isPlausibleKey(key)) throw new GroqError('no_key');
  if (blob.size > kMaxAudioBytes) throw new GroqError('too_large');
  const form = new FormData();
  form.append('file', blob, 'grabacion.wav');
  form.append('model', kTranscriptionModel);
  form.append('language', language);
  const data = await send(`${kGroqBase}/audio/transcriptions`, {
    method: 'POST',
    // No Content-Type: the browser adds the multipart boundary itself.
    headers: { Authorization: `Bearer ${key.trim()}` },
    body: form,
  }, { signal, fetchFn });
  const text = typeof data?.text === 'string' ? data.text.trim() : '';
  if (!text) throw new GroqError('empty');
  return text;
}

const kSummaryPrompt = 'Eres un asistente que resume notas de voz transcritas. Responde siempre en español. '
  + 'Escribe un resumen breve en 3 a 6 viñetas con las ideas principales y las tareas pendientes, '
  + 'si las hay. No añadas datos que no estén en el texto.';

export async function summarize(text, key, { signal, fetchFn = fetch } = {}) {
  if (!isPlausibleKey(key)) throw new GroqError('no_key');
  if (!text?.trim()) throw new GroqError('empty');
  const request = {
    method: 'POST',
    headers: { Authorization: `Bearer ${key.trim()}`, 'Content-Type': 'application/json' },
    body: JSON.stringify({
      model: kSummaryModel,
      messages: [
        { role: 'system', content: kSummaryPrompt },
        { role: 'user', content: text },
      ],
      temperature: 0.3,
      // gpt-oss reasons first and those tokens count here, so the limit leaves room for both.
      reasoning_effort: 'low',
      max_completion_tokens: 2048,
    }),
  };
  let data;
  try {
    data = await send(`${kGroqBase}/chat/completions`, request, { signal, fetchFn });
  } catch (error) {
    // Here a 413 is the token limit of the request, not the size of an audio file.
    if (error.code === 'too_large') throw new GroqError('text_too_long', error.status);
    throw error;
  }
  const summary = data?.choices?.[0]?.message?.content;
  if (typeof summary !== 'string' || !summary.trim()) throw new GroqError('empty_summary');
  return summary.trim();
}

// Even reading window.localStorage can throw (blocked cookies, some WebViews), so the page asks
// for it here instead of touching the global directly. Null when there is no storage.
export function browserStorage(win = globalThis) {
  try {
    return win.localStorage ?? null;
  } catch {
    return null;
  }
}

// localStorage may refuse (private mode, blocked storage): the key is then just not remembered.
export function loadKey(storage) {
  try {
    return storage?.getItem(kKeyStorageName)?.trim() ?? '';
  } catch {
    return '';
  }
}

export function saveKey(storage, key) {
  if (!isPlausibleKey(key)) return;
  try {
    storage.setItem(kKeyStorageName, key.trim());
  } catch {
    // Not remembered; the user pastes it again next time.
  }
}

export function forgetKey(storage) {
  try {
    storage.removeItem(kKeyStorageName);
  } catch {
    // Nothing to forget if nothing could be stored.
  }
}

// The text file offered for download: summary first, then the full transcript.
export function buildNotesText({ summary, transcript }) {
  const parts = [];
  if (summary) parts.push(`Resumen\n${summary}`);
  parts.push(`Transcripción\n${transcript}`);
  return `${parts.join('\n\n')}\n`;
}
