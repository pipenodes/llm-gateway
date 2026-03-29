import { describe, it, expect, beforeEach } from 'vitest';
import { http, HttpResponse } from 'msw';
import { server } from 'mocks/server';
import { fetcher, apiFetch, ApiError } from './fetcher';

describe('ApiError', () => {
  it('cria instância com status e mensagem', () => {
    const err = new ApiError('Não encontrado', 404);
    expect(err.message).toBe('Não encontrado');
    expect(err.status).toBe(404);
    expect(err.name).toBe('ApiError');
  });

  it('armazena traceId opcional', () => {
    const err = new ApiError('Erro', 500, 'trace-abc-123');
    expect(err.traceId).toBe('trace-abc-123');
  });
});

describe('fetcher', () => {
  beforeEach(() => {
    localStorage.clear();
  });

  it('retorna JSON para resposta 200', async () => {
    server.use(
      http.get('/api/test', () => HttpResponse.json({ ok: true }))
    );
    const result = await fetcher<{ ok: boolean }>('/api/test');
    expect(result.ok).toBe(true);
  });

  it('lança ApiError para resposta 404', async () => {
    server.use(
      http.get('/api/missing', () => HttpResponse.json({ message: 'Não encontrado' }, { status: 404 }))
    );
    await expect(fetcher('/api/missing')).rejects.toBeInstanceOf(ApiError);
    await expect(fetcher('/api/missing')).rejects.toMatchObject({ status: 404 });
  });

  it('lança ApiError para resposta 500', async () => {
    server.use(
      http.get('/api/error', () => HttpResponse.json({ message: 'Erro interno' }, { status: 500 }))
    );
    const err = await fetcher('/api/error').catch((e) => e);
    expect(err).toBeInstanceOf(ApiError);
    expect(err.status).toBe(500);
    expect(err.message).toBe('Erro interno');
  });

  it('retorna undefined para 204 No Content', async () => {
    // fetcher usa GET — registra handler GET que retorna 204
    server.use(
      http.get('/api/no-content', () => new HttpResponse(null, { status: 204 }))
    );
    const result = await fetcher('/api/no-content');
    expect(result).toBeUndefined();
  });

  it('inclui Authorization header quando token está presente', async () => {
    let authHeader = '';
    server.use(
      http.get('/api/auth-test', ({ request }) => {
        authHeader = request.headers.get('Authorization') ?? '';
        return HttpResponse.json({ ok: true });
      })
    );
    localStorage.setItem('access_token', 'meu-token-123');
    await fetcher('/api/auth-test');
    expect(authHeader).toBe('Bearer meu-token-123');
  });

  it('não inclui Authorization header sem token', async () => {
    let authHeader: string | null = 'present';
    server.use(
      http.get('/api/no-auth', ({ request }) => {
        authHeader = request.headers.get('Authorization');
        return HttpResponse.json({ ok: true });
      })
    );
    await fetcher('/api/no-auth');
    expect(authHeader).toBeNull();
  });
});

describe('apiFetch', () => {
  it('envia corpo JSON no POST', async () => {
    let received: unknown;
    server.use(
      http.post('/api/create', async ({ request }) => {
        received = await request.json();
        return HttpResponse.json({ id: 'novo' }, { status: 201 });
      })
    );
    const result = await apiFetch<{ id: string }>('/api/create', {
      method: 'POST',
      body: { name: 'Teste', role: 'user' }
    });
    expect(result.id).toBe('novo');
    expect(received).toEqual({ name: 'Teste', role: 'user' });
  });

  it('envia PUT com dados corretos', async () => {
    let method = '';
    server.use(
      http.put('/api/update/123', async ({ request }) => {
        method = request.method;
        return HttpResponse.json({ updated: true });
      })
    );
    await apiFetch('/api/update/123', { method: 'PUT', body: { name: 'Atualizado' } });
    expect(method).toBe('PUT');
  });
});
