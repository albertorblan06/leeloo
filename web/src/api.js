// Client for the device API in docs/WEB-API.md.
export class ApiError extends Error {
  constructor(code, status) {
    super(code);
    this.code = code;
    this.status = status;
  }
}

const messages = {
  invalid_pin: 'PIN incorrecto. Revisa el número que se muestra en el dispositivo.',
  unauthorized: 'La sesión no es válida. Vuelve a introducir el PIN.',
  session_closed: 'La sesión terminó. Vuelve a elegir una opción en el dispositivo.',
  bad_name: 'El nombre del libro no es válido o es demasiado largo.',
  library_full: 'La biblioteca está llena. Borra un libro en el dispositivo.',
  too_large: 'El libro es demasiado grande para el dispositivo (máximo 1 MiB de texto).',
  invalid_utf8: 'El texto del libro no es UTF-8 válido.',
  invalid_format: 'El dispositivo no reconoce el formato del libro.',
  no_space: 'No hay espacio libre suficiente en el dispositivo.',
  storage_error: 'El dispositivo no pudo completar la operación. Inténtalo de nuevo.',
  busy: 'El dispositivo está ocupado con otra operación. Inténtalo de nuevo en unos segundos.',
  no_recording: 'No hay ninguna grabación pendiente en el dispositivo.',
  invalid_audio: 'La grabación recibida está incompleta o no es válida. Sigue en el dispositivo: inténtalo de nuevo.',
  size_mismatch: 'El dispositivo conserva la grabación porque el tamaño recibido no coincide. Inténtalo de nuevo.',
  network: 'No se pudo conectar con el dispositivo. Comprueba la conexión Wi-Fi.',
  aborted: 'Operación cancelada.',
};

export function errorMessage(code) {
  return messages[code] ?? 'Error desconocido. La operación no se ha completado.';
}

export function errorFromResponse(status, bodyText) {
  let code;
  try {
    code = JSON.parse(bodyText).error;
  } catch {
    // Non-JSON body (e.g. an HTML error page): treated as an unknown error.
  }
  return new ApiError(typeof code === 'string' ? code : 'unknown', status);
}

export async function authenticate(pin, fetchFn = fetch) {
  let res;
  try {
    res = await fetchFn('/api/auth', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ pin }),
    });
  } catch {
    throw new ApiError('network', 0);
  }
  if (!res.ok) {
    throw errorFromResponse(res.status, await res.text());
  }
}

// XMLHttpRequest instead of fetch because fetch has no upload progress events.
export function uploadBook(name, text, { onProgress, signal, xhrFactory = () => new XMLHttpRequest() } = {}) {
  return new Promise((resolve, reject) => {
    if (signal?.aborted) {
      reject(new ApiError('aborted', 0));
      return;
    }
    const xhr = xhrFactory();
    xhr.open('POST', '/api/books');
    xhr.setRequestHeader('Content-Type', 'text/plain; charset=utf-8');
    xhr.setRequestHeader('X-Book-Name', encodeURIComponent(name));
    xhr.upload.onprogress = (e) => {
      if (e.lengthComputable) {
        onProgress?.(e.loaded / e.total);
      }
    };
    xhr.onload = () => {
      if (xhr.status === 201) {
        try {
          resolve(JSON.parse(xhr.responseText));
        } catch {
          reject(new ApiError('unknown', xhr.status));
        }
      } else {
        reject(errorFromResponse(xhr.status, xhr.responseText));
      }
    };
    xhr.onerror = () => reject(new ApiError('network', 0));
    xhr.onabort = () => reject(new ApiError('aborted', 0));
    signal?.addEventListener('abort', () => xhr.abort(), { once: true });
    xhr.send(text);
  });
}

// A WAV written by the device: 44-byte header, then exactly the PCM bytes it declares. The
// response is chunked, so there is no Content-Length to compare; the header is the check
// that the whole file arrived. `header` is the first 44 bytes as an ArrayBuffer.
export function wavIsComplete(header, size) {
  if (header.byteLength < 44 || size < 44) return false;
  const view = new DataView(header);
  const tag = (offset, text) => [...text].every((c, i) => view.getUint8(offset + i) === c.charCodeAt(0));
  return tag(0, 'RIFF') && tag(8, 'WAVEfmt ') && tag(36, 'data') &&
    view.getUint32(40, true) > 0 && size === 44 + view.getUint32(40, true) &&
    view.getUint32(4, true) === size - 8;
}

// Downloads the pending WAV. It never deletes it on the device: only acknowledgeRecording does,
// after this resolved with a complete file.
export async function fetchRecording({ signal, fetchFn = fetch } = {}) {
  let res;
  try {
    res = await fetchFn('/api/recording', { signal });
  } catch {
    throw new ApiError(signal?.aborted ? 'aborted' : 'network', 0);
  }
  if (!res.ok) {
    throw errorFromResponse(res.status, await res.text());
  }
  let blob;
  try {
    blob = await res.blob();
  } catch {
    throw new ApiError(signal?.aborted ? 'aborted' : 'network', res.status);
  }
  if (!wavIsComplete(await blob.slice(0, 44).arrayBuffer(), blob.size)) {
    throw new ApiError('invalid_audio', res.status);
  }
  return blob;
}

// Tells the device how many bytes arrived. It deletes its copy only if that is its file size.
export async function acknowledgeRecording(bytes, { signal, fetchFn = fetch } = {}) {
  let res;
  try {
    res = await fetchFn('/api/recording/ack', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ bytes }),
      signal,
    });
  } catch {
    throw new ApiError(signal?.aborted ? 'aborted' : 'network', 0);
  }
  if (!res.ok) {
    throw errorFromResponse(res.status, await res.text());
  }
  // A 200 means the device already deleted its copy; an unreadable body must not say otherwise.
  try {
    return await res.json();
  } catch {
    return { deleted: true };
  }
}
