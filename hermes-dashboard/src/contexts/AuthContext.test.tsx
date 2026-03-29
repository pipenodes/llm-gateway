import { describe, it, expect, beforeEach, vi } from 'vitest';
import { renderHook, act, waitFor } from '@testing-library/react';
import type { ReactNode } from 'react';
import { AuthProvider, useAuth } from './AuthContext';
import { EmpresaAtivaProvider } from './EmpresaAtivaContext';

const wrapper = ({ children }: { children: ReactNode }) => (
  <EmpresaAtivaProvider>
    <AuthProvider>{children}</AuthProvider>
  </EmpresaAtivaProvider>
);

describe('AuthContext', () => {
  beforeEach(() => {
    localStorage.clear();
    sessionStorage.clear();
    Object.defineProperty(window, 'location', {
      writable: true,
      value: { ...window.location, replace: vi.fn() }
    });
  });

  describe('estado inicial', () => {
    it('começa não autenticado quando não há token', async () => {
      const { result } = renderHook(() => useAuth(), { wrapper });
      await waitFor(() => expect(result.current.isLoading).toBe(false));
      expect(result.current.isAuthenticated).toBe(false);
      expect(result.current.user).toBeNull();
    });

    it('restaura sessão de token válido no localStorage', async () => {
      const payload = { userId: 'usr-002', isSuperAdmin: false, exp: Date.now() + 8 * 60 * 60 * 1000 };
      localStorage.setItem('access_token', btoa(JSON.stringify(payload)));

      const { result } = renderHook(() => useAuth(), { wrapper });
      await waitFor(() => expect(result.current.isLoading).toBe(false));

      expect(result.current.isAuthenticated).toBe(true);
      expect(result.current.user?.role).toBe('admin');
    });

    it('ignora token expirado no localStorage', async () => {
      const expired = btoa(JSON.stringify({ userId: 'usr-001', isSuperAdmin: true, exp: Date.now() - 1000 }));
      localStorage.setItem('access_token', expired);

      const { result } = renderHook(() => useAuth(), { wrapper });
      await waitFor(() => expect(result.current.isLoading).toBe(false));

      expect(result.current.isAuthenticated).toBe(false);
    });
  });

  describe('login', () => {
    it('autentica com credenciais válidas e armazena roleLabel', async () => {
      const { result } = renderHook(() => useAuth(), { wrapper });
      await waitFor(() => expect(result.current.isLoading).toBe(false));

      await act(async () => {
        await result.current.login('gerente@empresa.com', 'gerente123');
      });

      expect(result.current.isAuthenticated).toBe(true);
      expect(result.current.user?.email).toBe('gerente@empresa.com');
      expect(result.current.user?.role).toBe('gerente');
      expect(result.current.user?.roleLabel).toBe('Gerente');
      expect(localStorage.getItem('access_token')).toBeTruthy();
    });

    it('super_admin tem role e label corretos', async () => {
      const { result } = renderHook(() => useAuth(), { wrapper });
      await waitFor(() => expect(result.current.isLoading).toBe(false));
      await act(async () => { await result.current.login('superadmin@empresa.com', 'super123'); });
      expect(result.current.user?.roleLabel).toBe('Administrador do sistema');
      expect(result.current.user?.id).toBe('usr-001');
      expect(result.current.user?.role).toBe('super_admin');
    });

    it('lança erro para credenciais inválidas', async () => {
      const { result } = renderHook(() => useAuth(), { wrapper });
      await waitFor(() => expect(result.current.isLoading).toBe(false));

      let threw = false;
      try {
        await act(async () => { await result.current.login('x@x.com', 'errado'); });
      } catch {
        threw = true;
      }
      expect(threw).toBe(true);
      expect(result.current.isAuthenticated).toBe(false);
    });
  });

  describe('logout', () => {
    it('limpa estado e localStorage', async () => {
      const { result } = renderHook(() => useAuth(), { wrapper });
      await waitFor(() => expect(result.current.isLoading).toBe(false));

      await act(async () => { await result.current.login('operador@empresa.com', 'operador123'); });
      expect(result.current.isAuthenticated).toBe(true);

      act(() => { result.current.logout(); });

      expect(localStorage.getItem('access_token')).toBeNull();
    });
  });

  describe('hasRole', () => {
    it('retorna true quando o role bate', async () => {
      const { result } = renderHook(() => useAuth(), { wrapper });
      await waitFor(() => expect(result.current.isLoading).toBe(false));
      await act(async () => { await result.current.login('admin@alpha.com', 'admin123'); });

      expect(result.current.hasRole('admin')).toBe(true);
      expect(result.current.hasRole('super_admin')).toBe(false);
      expect(result.current.hasRole(['admin', 'gerente'])).toBe(true);
    });

    it('retorna false sem usuário autenticado', async () => {
      const { result } = renderHook(() => useAuth(), { wrapper });
      await waitFor(() => expect(result.current.isLoading).toBe(false));

      expect(result.current.hasRole('admin')).toBe(false);
    });
  });

  describe('hasPermission', () => {
    it('super_admin tem acesso total (*)', async () => {
      const { result } = renderHook(() => useAuth(), { wrapper });
      await waitFor(() => expect(result.current.isLoading).toBe(false));
      await act(async () => { await result.current.login('superadmin@empresa.com', 'super123'); });

      expect(result.current.hasPermission('qualquer:coisa')).toBe(true);
    });

    it('visitante tem dashboard:read', async () => {
      const { result } = renderHook(() => useAuth(), { wrapper });
      await waitFor(() => expect(result.current.isLoading).toBe(false));
      await act(async () => { await result.current.login('visitante@empresa.com', 'visitante123'); });

      expect(result.current.hasPermission('dashboard:read')).toBe(true);
      expect(result.current.hasPermission('users:write')).toBe(false);
    });
  });

  describe('useAuth fora do provider', () => {
    it('lança erro descritivo', () => {
      expect(() => renderHook(() => useAuth())).toThrow('useAuth must be used within AuthProvider');
    });
  });
});
