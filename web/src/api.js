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
  no_recording: 'No hay ningún audio grabado pendiente en el dispositivo.',
  invalid_audio: 'El dispositivo no ha enviado un WAV válido.',
  busy: 'El dispositivo está ocupado con otra operación de almacenamiento.',
  library_full: 'La biblioteca está llena. Borra un libro en el dispositivo.',
  too_large: 'El libro es demasiado grande para el dispositivo (máximo 1 MiB de texto).',
  invalid_utf8: 'El texto del libro no es UTF-8 válido.',
  invalid_format: 'El dispositivo no reconoce el formato del libro.',
  no_space: 'No hay espacio libre suficiente en el dispositivo.',
  storage_error: 'El dispositivo no pudo completar la operación. Inténtalo de nuevo.',
  network: 'No se pudo conectar con el dispositivo. Comprueba la conexión Wi-Fi.',
  aborted: 'Operación cancelada.',
};

export function errorMessage(code) {
  return messages[code] ?? 'No se pudo completar la operación.';
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

export async function fetchRecording({ signal, fetchFn = fetch } = {}) {
  let res;
  try {
    res = await fetchFn('/api/recording', { signal });
  } catch {
    throw new ApiError(signal?.aborted ? 'aborted' : 'network', 0);
  }
  if (!res.ok) throw errorFromResponse(res.status, await res.text());

  let blob;
  try {
    blob = await res.blob();
  } catch {
    throw new ApiError(signal?.aborted ? 'aborted' : 'network', res.status);
  }
  if (blob.size < 44 || blob.type !== 'audio/wav') {
    throw new ApiError('invalid_audio', res.status);
  }
  return blob;
}

export async function acknowledgeRecording({ signal, fetchFn = fetch } = {}) {
  let res;
  try {
    res = await fetchFn('/api/recording/ack', { method: 'POST', signal });
  } catch {
    throw new ApiError(signal?.aborted ? 'aborted' : 'network', 0);
  }
  if (!res.ok) throw errorFromResponse(res.status, await res.text());
  return res.json();
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
