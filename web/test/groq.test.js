import { describe, it, expect, vi } from 'vitest';
import {
  GroqError, browserStorage, buildNotesText, forgetKey, groqErrorMessage, isPlausibleKey, kMaxAudioBytes,
  kSummaryModel, kTranscriptionModel, loadKey, saveKey, summarize, transcribe,
} from '../src/groq.js';

const KEY = 'gsk_abcdefghijklmnopqrstuvwxyz0123456789';

const reply = (status, body) => ({
  ok: status >= 200 && status < 300,
  status,
  json: async () => body,
  text: async () => JSON.stringify(body),
});

const audio = (size = 1000) => new Blob([new Uint8Array(size)], { type: 'audio/wav' });

describe('isPlausibleKey', () => {
  it('accepts a Groq-looking key', () => {
    expect(isPlausibleKey(KEY)).toBe(true);
  });

  it('rejects empty, short, wrongly prefixed and whitespace-containing keys', () => {
    for (const bad of ['', 'gsk_', 'sk-abcdefghijklmnopqrstuvwxyz0123', 'gsk_abc def ghijklmnopqrstuvwxyz0123', null, undefined, 5]) {
      expect(isPlausibleKey(bad)).toBe(false);
    }
  });

  it('rejects keys with characters that could not go in a header', () => {
    // A zero-width space or a typographic dash pasted with the key: fetch would throw and the
    // user would be told there is no internet.
    expect(isPlausibleKey(`gsk_abcdefghijklmnop\u200Bqrstuvwxyz`)).toBe(false);
    expect(isPlausibleKey('gsk_abcdefghijklmnop\u2013qrstuvwxyz')).toBe(false);
    expect(isPlausibleKey('gsk_abcdefghijklmnopñqrstuvwxyz')).toBe(false);
  });

  it('ignores surrounding whitespace pasted with the key', () => {
    expect(isPlausibleKey(`  ${KEY}\n`)).toBe(true);
  });
});

describe('groqErrorMessage', () => {
  it('has a Spanish message for every code the client can raise', () => {
    for (const code of ['no_key', 'invalid_key', 'network', 'too_large', 'rate_limit', 'model_not_found',
      'server', 'bad_request', 'empty', 'empty_summary', 'text_too_long', 'aborted']) {
      expect(groqErrorMessage(code)).not.toMatch(/desconocido/i);
    }
  });

  it('tells the user what to do when there is no internet', () => {
    expect(groqErrorMessage('network')).toMatch(/datos móviles/i);
  });

  it('falls back to a generic message', () => {
    expect(groqErrorMessage('weird')).toMatch(/desconocido/i);
  });

});

describe('transcribe', () => {
  it('posts the audio as multipart with the model and Spanish, using the key only as a bearer token', async () => {
    const fetchFn = vi.fn().mockResolvedValue(reply(200, { text: '  hola mundo \n' }));
    const text = await transcribe(audio(), KEY, { fetchFn });
    expect(text).toBe('hola mundo');
    const [url, init] = fetchFn.mock.calls[0];
    expect(url).toBe('https://api.groq.com/openai/v1/audio/transcriptions');
    expect(init.method).toBe('POST');
    expect(init.headers.Authorization).toBe(`Bearer ${KEY}`);
    expect(init.headers['Content-Type']).toBeUndefined();  // the browser sets the multipart boundary
    expect(init.body.get('model')).toBe(kTranscriptionModel);
    expect(init.body.get('language')).toBe('es');
    const file = init.body.get('file');
    expect(file).toBeInstanceOf(Blob);
    expect(file.name).toBe('grabacion.wav');
    expect(file.type).toBe('audio/wav');
  });

  it('never puts the key in the URL or in the form', async () => {
    const fetchFn = vi.fn().mockResolvedValue(reply(200, { text: 'x' }));
    await transcribe(audio(), KEY, { fetchFn });
    const [url, init] = fetchFn.mock.calls[0];
    expect(url).not.toContain(KEY);
    for (const [, value] of init.body.entries()) expect(String(value)).not.toContain(KEY);
  });

  it('trims the key it is given', async () => {
    const fetchFn = vi.fn().mockResolvedValue(reply(200, { text: 'x' }));
    await transcribe(audio(), `  ${KEY}  `, { fetchFn });
    expect(fetchFn.mock.calls[0][1].headers.Authorization).toBe(`Bearer ${KEY}`);
  });

  it('refuses without a plausible key, before any request', async () => {
    const fetchFn = vi.fn();
    await expect(transcribe(audio(), '', { fetchFn })).rejects.toMatchObject({ code: 'no_key' });
    await expect(transcribe(audio(), 'hola', { fetchFn })).rejects.toMatchObject({ code: 'no_key' });
    expect(fetchFn).not.toHaveBeenCalled();
  });

  it('refuses a recording over the 25 MB free-tier limit, before any request', async () => {
    const fetchFn = vi.fn();
    const big = { size: kMaxAudioBytes + 1 };
    await expect(transcribe(big, KEY, { fetchFn })).rejects.toMatchObject({ code: 'too_large' });
    expect(fetchFn).not.toHaveBeenCalled();
  });

  it('maps the HTTP statuses to codes', async () => {
    const cases = [[401, 'invalid_key'], [413, 'too_large'], [429, 'rate_limit'], [400, 'bad_request'],
      [500, 'server'], [503, 'server']];
    for (const [status, code] of cases) {
      const fetchFn = vi.fn().mockResolvedValue(reply(status, { error: { message: 'x' } }));
      await expect(transcribe(audio(), KEY, { fetchFn })).rejects.toMatchObject({ code, status });
    }
  });

  it('recognises a model that Groq no longer serves', async () => {
    const fetchFn = vi.fn().mockResolvedValue(reply(404, { error: { code: 'model_not_found', message: 'gone' } }));
    await expect(transcribe(audio(), KEY, { fetchFn })).rejects.toMatchObject({ code: 'model_not_found' });
  });

  it('recognises a retired model whatever status Groq answers with', async () => {
    for (const [status, code] of [[400, 'model_decommissioned'], [404, 'model_decommissioned'], [400, 'model_not_found']]) {
      const fetchFn = vi.fn().mockResolvedValue(reply(status, { error: { code, message: 'retired' } }));
      await expect(transcribe(audio(), KEY, { fetchFn })).rejects.toMatchObject({ code: 'model_not_found' });
    }
  });

  it('keeps a 404 without that code as a bad request, and a 403 as a key problem', async () => {
    const notFound = vi.fn().mockResolvedValue(reply(404, { error: { code: 'something_else' } }));
    await expect(transcribe(audio(), KEY, { fetchFn: notFound })).rejects.toMatchObject({ code: 'bad_request' });
    const forbidden = vi.fn().mockResolvedValue(reply(403, { error: { message: 'x' } }));
    await expect(transcribe(audio(), KEY, { fetchFn: forbidden })).rejects.toMatchObject({ code: 'invalid_key' });
  });

  it('uses the status alone when the error body is not JSON', async () => {
    const html = { ok: false, status: 429, json: async () => { throw new SyntaxError('<html>'); } };
    await expect(transcribe(audio(), KEY, { fetchFn: vi.fn().mockResolvedValue(html) }))
      .rejects.toMatchObject({ code: 'rate_limit', status: 429 });
  });

  it('reports an abort that happens while the answer is being read as cancelled, not as a server fault', async () => {
    const controller = new AbortController();
    const slow = { ok: true, status: 200, json: async () => { controller.abort(); throw new DOMException('aborted', 'AbortError'); } };
    await expect(transcribe(audio(), KEY, { fetchFn: vi.fn().mockResolvedValue(slow), signal: controller.signal }))
      .rejects.toMatchObject({ code: 'aborted' });
  });

  it('reports a missing network, or an abort when the signal fired', async () => {
    const down = vi.fn().mockRejectedValue(new TypeError('Failed to fetch'));
    await expect(transcribe(audio(), KEY, { fetchFn: down })).rejects.toMatchObject({ code: 'network' });
    const controller = new AbortController();
    controller.abort();
    await expect(transcribe(audio(), KEY, { fetchFn: down, signal: controller.signal }))
      .rejects.toMatchObject({ code: 'aborted' });
  });

  it('reports a recording with no words', async () => {
    const fetchFn = vi.fn().mockResolvedValue(reply(200, { text: '   ' }));
    await expect(transcribe(audio(), KEY, { fetchFn })).rejects.toMatchObject({ code: 'empty' });
  });

  it('reports an answer that is not JSON as a server problem', async () => {
    const fetchFn = vi.fn().mockResolvedValue({ ok: true, status: 200, json: async () => { throw new SyntaxError('x'); } });
    await expect(transcribe(audio(), KEY, { fetchFn })).rejects.toMatchObject({ code: 'server' });
  });

  it('does not leak the key in the error it raises', async () => {
    const fetchFn = vi.fn().mockResolvedValue(reply(401, { error: { message: `bad key ${KEY}` } }));
    const error = await transcribe(audio(), KEY, { fetchFn }).catch((e) => e);
    expect(error).toBeInstanceOf(GroqError);
    expect(`${error.message} ${error.code}`).not.toContain(KEY);
  });
});

describe('summarize', () => {
  const answer = (content) => reply(200, { choices: [{ message: { content } }] });

  it('asks the chat model for a Spanish summary of the transcript', async () => {
    const fetchFn = vi.fn().mockResolvedValue(answer('  - idea uno\n- idea dos \n'));
    const summary = await summarize('Comprar pan y llamar a Ana.', KEY, { fetchFn });
    expect(summary).toBe('- idea uno\n- idea dos');
    const [url, init] = fetchFn.mock.calls[0];
    expect(url).toBe('https://api.groq.com/openai/v1/chat/completions');
    expect(init.headers.Authorization).toBe(`Bearer ${KEY}`);
    expect(init.headers['Content-Type']).toBe('application/json');
    const body = JSON.parse(init.body);
    expect(body.model).toBe(kSummaryModel);
    expect(body.reasoning_effort).toBe('low');
    expect(body.max_completion_tokens).toBeGreaterThan(0);
    expect(body.messages[0].role).toBe('system');
    expect(body.messages[0].content).toMatch(/español/i);
    expect(body.messages[1]).toEqual({ role: 'user', content: 'Comprar pan y llamar a Ana.' });
  });

  it('never puts the key in the URL or the body', async () => {
    const fetchFn = vi.fn().mockResolvedValue(answer('ok'));
    await summarize('texto', KEY, { fetchFn });
    const [url, init] = fetchFn.mock.calls[0];
    expect(url).not.toContain(KEY);
    expect(init.body).not.toContain(KEY);
  });

  it('refuses without a key or without text, before any request', async () => {
    const fetchFn = vi.fn();
    await expect(summarize('texto', '', { fetchFn })).rejects.toMatchObject({ code: 'no_key' });
    await expect(summarize('   ', KEY, { fetchFn })).rejects.toMatchObject({ code: 'empty' });
    expect(fetchFn).not.toHaveBeenCalled();
  });

  it('maps errors like transcribe does', async () => {
    for (const [status, code] of [[401, 'invalid_key'], [429, 'rate_limit'], [500, 'server']]) {
      const fetchFn = vi.fn().mockResolvedValue(reply(status, { error: { message: 'x' } }));
      await expect(summarize('texto', KEY, { fetchFn })).rejects.toMatchObject({ code });
    }
    const down = vi.fn().mockRejectedValue(new TypeError('Failed to fetch'));
    await expect(summarize('texto', KEY, { fetchFn: down })).rejects.toMatchObject({ code: 'network' });
  });

  it('says the text is too long when the request is refused as too large', async () => {
    const fetchFn = vi.fn().mockResolvedValue(reply(413, { error: { message: 'tokens' } }));
    await expect(summarize('texto', KEY, { fetchFn })).rejects.toMatchObject({ code: 'text_too_long', status: 413 });
  });

  it('recognises a retired chat model', async () => {
    const fetchFn = vi.fn().mockResolvedValue(reply(400, { error: { code: 'model_decommissioned' } }));
    await expect(summarize('texto', KEY, { fetchFn })).rejects.toMatchObject({ code: 'model_not_found' });
  });

  it('reports an answer with no content', async () => {
    for (const body of [{ choices: [] }, { choices: [{ message: {} }] }, {}, { choices: [{ message: { content: '  ' } }] }]) {
      const fetchFn = vi.fn().mockResolvedValue(reply(200, body));
      await expect(summarize('texto', KEY, { fetchFn })).rejects.toMatchObject({ code: 'empty_summary' });
    }
  });
});

describe('key storage', () => {
  const fakeStorage = () => {
    const data = new Map();
    return {
      getItem: (k) => (data.has(k) ? data.get(k) : null),
      setItem: (k, v) => data.set(k, v),
      removeItem: (k) => data.delete(k),
    };
  };

  it('saves, loads and forgets the key', () => {
    const storage = fakeStorage();
    expect(loadKey(storage)).toBe('');
    saveKey(storage, `  ${KEY}\n`);
    expect(loadKey(storage)).toBe(KEY);
    forgetKey(storage);
    expect(loadKey(storage)).toBe('');
  });

  it('does not store something that is not a plausible key', () => {
    const storage = fakeStorage();
    saveKey(storage, 'hola');
    expect(loadKey(storage)).toBe('');
  });

  it('treats a stored value of only spaces as no key', () => {
    const storage = fakeStorage();
    storage.setItem('leeloo_groq_key', '   ');
    expect(loadKey(storage)).toBe('');
  });

  it('survives a browser that refuses storage (private mode)', () => {
    const broken = {
      getItem: () => { throw new Error('denied'); },
      setItem: () => { throw new Error('denied'); },
      removeItem: () => { throw new Error('denied'); },
    };
    expect(loadKey(broken)).toBe('');
    expect(() => saveKey(broken, KEY)).not.toThrow();
    expect(() => forgetKey(broken)).not.toThrow();
    expect(loadKey(undefined)).toBe('');
  });
});

describe('browserStorage', () => {
  it('returns the storage of the window', () => {
    const storage = { getItem() {} };
    expect(browserStorage({ localStorage: storage })).toBe(storage);
  });

  it('returns null when merely reading localStorage throws (blocked cookies)', () => {
    const win = { get localStorage() { throw new DOMException('denied', 'SecurityError'); } };
    expect(browserStorage(win)).toBeNull();
  });

  it('returns null when there is none, and the key helpers then do nothing', () => {
    const storage = browserStorage({});
    expect(storage).toBeNull();
    expect(loadKey(storage)).toBe('');
    expect(() => saveKey(storage, KEY)).not.toThrow();
    expect(() => forgetKey(storage)).not.toThrow();
  });
});

describe('buildNotesText', () => {
  it('puts the summary first and then the transcript, with headings', () => {
    const text = buildNotesText({ summary: '- uno', transcript: 'texto largo' });
    expect(text.indexOf('Resumen')).toBeLessThan(text.indexOf('Transcripción'));
    expect(text).toContain('- uno');
    expect(text).toContain('texto largo');
  });

  it('works with the transcript only, when the summary failed', () => {
    const text = buildNotesText({ summary: '', transcript: 'solo texto' });
    expect(text).not.toContain('Resumen');
    expect(text).toContain('Transcripción');
    expect(text).toContain('solo texto');
  });

  it('ends with a newline', () => {
    expect(buildNotesText({ summary: 'a', transcript: 'b' }).endsWith('\n')).toBe(true);
  });
});
