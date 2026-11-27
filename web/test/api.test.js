import { describe, it, expect, vi } from 'vitest';
import {
  ApiError, acknowledgeRecording, authenticate, errorFromResponse, errorMessage, fetchRecording,
  uploadBook, wavIsComplete,
} from '../src/api.js';

class FakeXhr {
  constructor() {
    this.headers = {};
    this.upload = {};
    this.status = 0;
    this.responseText = '';
    FakeXhr.last = this;
  }
  open(method, url) {
    this.method = method;
    this.url = url;
  }
  setRequestHeader(k, v) {
    this.headers[k] = v;
  }
  send(body) {
    this.body = body;
  }
  abort() {
    this.onabort?.();
  }
  respond(status, text) {
    this.status = status;
    this.responseText = text;
    this.onload();
  }
}
const xhrFactory = () => new FakeXhr();

describe('errorMessage', () => {
  it('returns a Spanish message for every contract error code', () => {
    for (const code of ['invalid_pin', 'unauthorized', 'session_closed', 'bad_name', 'library_full',
      'too_large', 'invalid_utf8', 'invalid_format', 'no_space', 'storage_error', 'network', 'aborted',
      'busy', 'no_recording', 'invalid_audio', 'size_mismatch']) {
      expect(errorMessage(code)).not.toMatch(/desconocido/i);
    }
  });

  it('falls back to a generic message for an unknown code', () => {
    expect(errorMessage('weird')).toMatch(/desconocido/i);
  });
});

describe('errorFromResponse', () => {
  it('reads the error code from the JSON body', () => {
    const e = errorFromResponse(422, '{"error":"invalid_format"}');
    expect(e).toBeInstanceOf(ApiError);
    expect(e.code).toBe('invalid_format');
    expect(e.status).toBe(422);
  });

  it('uses unknown for a non-JSON body', () => {
    expect(errorFromResponse(500, '<html>').code).toBe('unknown');
  });

  it('uses unknown when error is not a string', () => {
    expect(errorFromResponse(500, '{"error":5}').code).toBe('unknown');
  });
});

describe('authenticate', () => {
  it('posts the PIN as JSON', async () => {
    const fetchFn = vi.fn().mockResolvedValue({ ok: true });
    await authenticate('1234', fetchFn);
    expect(fetchFn).toHaveBeenCalledWith('/api/auth', expect.objectContaining({
      method: 'POST',
      body: '{"pin":"1234"}',
    }));
  });

  it('throws the device error code on a rejected PIN', async () => {
    const fetchFn = vi.fn().mockResolvedValue({ ok: false, status: 401, text: async () => '{"error":"invalid_pin"}' });
    await expect(authenticate('0000', fetchFn)).rejects.toMatchObject({ code: 'invalid_pin', status: 401 });
  });

  it('maps a failed connection to the network error', async () => {
    const fetchFn = vi.fn().mockRejectedValue(new TypeError('fail'));
    await expect(authenticate('1234', fetchFn)).rejects.toMatchObject({ code: 'network' });
  });
});

describe('uploadBook', () => {
  it('sends the text with the percent-encoded name header', async () => {
    const p = uploadBook('Ñandú ¿?', 'TITLE: x', { xhrFactory });
    const x = FakeXhr.last;
    x.respond(201, '{"id":"a","bytes":8}');
    await expect(p).resolves.toEqual({ id: 'a', bytes: 8 });
    expect(x.method).toBe('POST');
    expect(x.url).toBe('/api/books');
    expect(x.headers['X-Book-Name']).toBe(encodeURIComponent('Ñandú ¿?'));
    expect(x.headers['Content-Type']).toBe('text/plain; charset=utf-8');
    expect(x.body).toBe('TITLE: x');
  });

  it('reports upload progress as a 0..1 fraction', () => {
    const onProgress = vi.fn();
    uploadBook('n', 'x', { xhrFactory, onProgress }).catch(() => {});
    FakeXhr.last.upload.onprogress({ lengthComputable: true, loaded: 50, total: 200 });
    expect(onProgress).toHaveBeenCalledWith(0.25);
  });

  it('ignores progress events without a computable length', () => {
    const onProgress = vi.fn();
    uploadBook('n', 'x', { xhrFactory, onProgress }).catch(() => {});
    FakeXhr.last.upload.onprogress({ lengthComputable: false, loaded: 1, total: 0 });
    expect(onProgress).not.toHaveBeenCalled();
  });

  it('rejects with the device error code on a non-201 response', async () => {
    const p = uploadBook('n', 'x', { xhrFactory });
    FakeXhr.last.respond(413, '{"error":"too_large"}');
    await expect(p).rejects.toMatchObject({ code: 'too_large', status: 413 });
  });

  it('rejects instead of hanging when a 201 body is not JSON', async () => {
    const p = uploadBook('n', 'x', { xhrFactory });
    FakeXhr.last.respond(201, 'ok');
    await expect(p).rejects.toMatchObject({ code: 'unknown', status: 201 });
  });

  it('rejects with network on a connection error', async () => {
    const p = uploadBook('n', 'x', { xhrFactory });
    FakeXhr.last.onerror();
    await expect(p).rejects.toMatchObject({ code: 'network' });
  });

  it('aborts the request and rejects with aborted when the signal fires', async () => {
    const ctrl = new AbortController();
    const p = uploadBook('n', 'x', { xhrFactory, signal: ctrl.signal });
    ctrl.abort();
    await expect(p).rejects.toMatchObject({ code: 'aborted' });
  });

  it('rejects immediately when the signal is already aborted', async () => {
    const ctrl = new AbortController();
    ctrl.abort();
    await expect(uploadBook('n', 'x', { xhrFactory, signal: ctrl.signal })).rejects.toMatchObject({ code: 'aborted' });
  });
});

// A WAV as the device writes it: mono 16-bit, `pcm` bytes of audio.
function wav(pcm, { riffSize, dataSize } = {}) {
  const buf = new ArrayBuffer(44 + pcm);
  const v = new DataView(buf);
  const put = (o, t) => [...t].forEach((c, i) => v.setUint8(o + i, c.charCodeAt(0)));
  put(0, 'RIFF');
  v.setUint32(4, riffSize ?? 36 + pcm, true);
  put(8, 'WAVEfmt ');
  v.setUint32(16, 16, true);
  v.setUint16(20, 1, true);
  v.setUint16(22, 1, true);
  v.setUint32(24, 8000, true);
  v.setUint32(28, 16000, true);
  v.setUint16(32, 2, true);
  v.setUint16(34, 16, true);
  put(36, 'data');
  v.setUint32(40, dataSize ?? pcm, true);
  return buf;
}

const response = (status, body, type = 'audio/wav') => ({
  ok: status >= 200 && status < 300,
  status,
  blob: async () => new Blob([body], { type }),
  text: async () => (typeof body === 'string' ? body : ''),
  json: async () => JSON.parse(body),
});

describe('wavIsComplete', () => {
  it('accepts a file whose size matches its header', () => {
    const buf = wav(1000);
    expect(wavIsComplete(buf.slice(0, 44), buf.byteLength)).toBe(true);
  });

  it('rejects a download cut short', () => {
    const buf = wav(1000);
    expect(wavIsComplete(buf.slice(0, 44), buf.byteLength - 2)).toBe(false);
  });

  it('rejects extra bytes, an empty data chunk and an inconsistent RIFF size', () => {
    const buf = wav(1000);
    expect(wavIsComplete(buf.slice(0, 44), buf.byteLength + 1)).toBe(false);
    const empty = wav(0);
    expect(wavIsComplete(empty.slice(0, 44), 44)).toBe(false);
    const bad = wav(1000, { riffSize: 5 });
    expect(wavIsComplete(bad.slice(0, 44), bad.byteLength)).toBe(false);
  });

  it('rejects something that is not a WAV, or less than a header', () => {
    expect(wavIsComplete(new ArrayBuffer(44), 44)).toBe(false);
    expect(wavIsComplete(new ArrayBuffer(10), 10)).toBe(false);
  });
});

describe('fetchRecording', () => {
  it('returns the blob of a complete WAV', async () => {
    const fetchFn = vi.fn().mockResolvedValue(response(200, wav(1000)));
    const blob = await fetchRecording({ fetchFn });
    expect(blob.size).toBe(1044);
    expect(fetchFn).toHaveBeenCalledWith('/api/recording', { signal: undefined });
  });

  it('maps a device error body to its code', async () => {
    const fetchFn = vi.fn().mockResolvedValue(response(404, '{"error":"no_recording"}'));
    await expect(fetchRecording({ fetchFn })).rejects.toMatchObject({ code: 'no_recording', status: 404 });
  });

  it('rejects a file whose size disagrees with its header as invalid_audio', async () => {
    const short = wav(1000).slice(0, 900);
    const fetchFn = vi.fn().mockResolvedValue(response(200, short));
    await expect(fetchRecording({ fetchFn })).rejects.toMatchObject({ code: 'invalid_audio' });
  });

  it('reports a network failure, or an abort when the signal fired', async () => {
    const down = vi.fn().mockRejectedValue(new TypeError('failed'));
    await expect(fetchRecording({ fetchFn: down })).rejects.toMatchObject({ code: 'network' });
    const controller = new AbortController();
    controller.abort();
    await expect(fetchRecording({ fetchFn: down, signal: controller.signal }))
      .rejects.toMatchObject({ code: 'aborted' });
  });

  it('reports a body that breaks while it is read', async () => {
    const broken = { ok: true, status: 200, blob: async () => { throw new TypeError('reset'); } };
    await expect(fetchRecording({ fetchFn: vi.fn().mockResolvedValue(broken) }))
      .rejects.toMatchObject({ code: 'network', status: 200 });
  });
});

describe('acknowledgeRecording', () => {
  it('posts the received byte count as JSON', async () => {
    const fetchFn = vi.fn().mockResolvedValue(response(200, '{"deleted":true}', 'application/json'));
    const result = await acknowledgeRecording(1044, { fetchFn });
    expect(result).toEqual({ deleted: true });
    const [url, init] = fetchFn.mock.calls[0];
    expect(url).toBe('/api/recording/ack');
    expect(init.method).toBe('POST');
    expect(JSON.parse(init.body)).toEqual({ bytes: 1044 });
  });

  it('treats an unreadable 200 body as deleted, since the device answered success', async () => {
    const fetchFn = vi.fn().mockResolvedValue({ ok: true, status: 200, json: async () => { throw new SyntaxError('x'); } });
    await expect(acknowledgeRecording(10, { fetchFn })).resolves.toEqual({ deleted: true });
  });

  it('maps a refusal to its code, so the user knows the audio is still on the device', async () => {
    const fetchFn = vi.fn().mockResolvedValue(response(409, '{"error":"size_mismatch"}', 'application/json'));
    await expect(acknowledgeRecording(10, { fetchFn })).rejects.toMatchObject({ code: 'size_mismatch', status: 409 });
  });

  it('reports a network failure', async () => {
    const down = vi.fn().mockRejectedValue(new TypeError('failed'));
    await expect(acknowledgeRecording(10, { fetchFn: down })).rejects.toMatchObject({ code: 'network' });
  });
});
