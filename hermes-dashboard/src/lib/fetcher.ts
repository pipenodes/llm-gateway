export class ApiError extends Error {
  status: number;
  traceId?: string;

  constructor(message: string, status: number, traceId?: string) {
    super(message);
    this.name = 'ApiError';
    this.status = status;
    this.traceId = traceId;
  }
}

const ADMIN_KEY_STORAGE = 'hermes_admin_key';

function getAuthHeaders(): HeadersInit {
  const key = sessionStorage.getItem(ADMIN_KEY_STORAGE);
  return {
    'Content-Type': 'application/json',
    Accept: 'application/json',
    ...(key && { Authorization: `Bearer ${key}` })
  };
}

function handleUnauthorized() {
  sessionStorage.removeItem(ADMIN_KEY_STORAGE);
  const redirectTo = encodeURIComponent(window.location.pathname + window.location.search);
  window.location.replace(`/login?redirectTo=${redirectTo}`);
}

export async function fetcher<T = unknown>(url: string): Promise<T> {
  const baseUrl = import.meta.env.VITE_API_URL ?? '';
  const fullUrl = url.startsWith('http') ? url : `${baseUrl}${url}`;

  const res = await fetch(fullUrl, { headers: getAuthHeaders() });

  if (res.status === 401) {
    handleUnauthorized();
    throw new ApiError('Chave invalida', 401);
  }

  if (!res.ok) {
    let message = `Erro ${res.status}`;
    let traceId: string | undefined;
    try {
      const body = await res.json();
      message = body.message ?? body.error ?? message;
      traceId = body.traceId ?? body.trace_id;
    } catch {
      // body is not JSON
    }
    throw new ApiError(message, res.status, traceId);
  }

  if (res.status === 204) return undefined as T;
  return res.json() as Promise<T>;
}

export async function apiFetch<T = unknown>(
  url: string,
  options: {
    method?: 'GET' | 'POST' | 'PUT' | 'PATCH' | 'DELETE';
    body?: unknown;
    headers?: HeadersInit;
  } = {}
): Promise<T> {
  const { method = 'GET', body, headers = {} } = options;
  const baseUrl = import.meta.env.VITE_API_URL ?? '';
  const fullUrl = url.startsWith('http') ? url : `${baseUrl}${url}`;

  const res = await fetch(fullUrl, {
    method,
    headers: { ...getAuthHeaders(), ...headers },
    ...(body !== undefined && { body: JSON.stringify(body) })
  });

  if (res.status === 401) {
    handleUnauthorized();
    throw new ApiError('Chave invalida', 401);
  }

  if (!res.ok) {
    let message = `Erro ${res.status}`;
    let traceId: string | undefined;
    try {
      const errBody = await res.json();
      message = errBody.message ?? errBody.error ?? message;
      traceId = errBody.traceId ?? errBody.trace_id;
    } catch { /* empty */ }
    throw new ApiError(message, res.status, traceId);
  }

  if (res.status === 204) return undefined as T;
  return res.json() as Promise<T>;
}
