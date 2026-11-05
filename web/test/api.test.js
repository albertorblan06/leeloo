import { describe, it, expect, vi } from 'vitest';
import { ApiError, authenticate, errorFromResponse, errorMessage, uploadBook } from '../src/api.js';

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
      'too_large', 'invalid_utf8', 'invalid_format', 'no_space', 'storage_error', 'network', 'aborted']) {
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
