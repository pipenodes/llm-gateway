import { createContext, useCallback, useContext, useEffect, useMemo, useState } from 'react';
import type { ReactNode } from 'react';
import * as authClient from 'lib/authClient';
import type { AuthServiceUser } from 'lib/authClient';

/**
 * HERMES-Dashboard auth supports two modes:
 *   - Admin key (OSS default): validates a static bearer key against /health
 *   - JWT (cloud/enterprise): uses the unified Pipeleap auth service
 *
 * When VITE_AUTH_URL is set, JWT mode is active.
 */
const USE_REAL_AUTH = !!import.meta.env.VITE_AUTH_URL;
const ADMIN_KEY_STORAGE = 'hermes_admin_key';

export interface AuthContextValue {
  /** In admin-key mode: the bearer key. In JWT mode: null. */
  adminKey: string | null;
  /** In JWT mode: the authenticated user. In admin-key mode: null. */
  serviceUser: AuthServiceUser | null;
  isAuthenticated: boolean;
  isLoading: boolean;
  /**
   * Unified login:
   *   - admin-key mode: pass key as `email`, password is ignored
   *   - JWT mode: email + password against auth service
   */
  login: (emailOrKey: string, password?: string) => Promise<void>;
  logout: () => void;
  /** True if the authenticated user has admin or owner role */
  isAdmin: boolean;
}

const AuthContext = createContext<AuthContextValue | null>(null);

export function AuthProvider({ children }: { children: ReactNode }) {
  const [adminKey, setAdminKey] = useState<string | null>(null);
  const [serviceUser, setServiceUser] = useState<AuthServiceUser | null>(null);
  const [isLoading, setIsLoading] = useState(true);

  // ── Restore session on mount ─────────────────────────────────────────────────
  useEffect(() => {
    if (USE_REAL_AUTH) {
      const session = authClient.loadStoredSession();
      if (session) {
        setServiceUser(session.user);
        authClient.me(session.accessToken)
          .then(setServiceUser)
          .catch(async () => {
            try {
              const tokens = await authClient.refreshTokens(session.refreshToken);
              const refreshed = await authClient.me(tokens.access_token);
              authClient.storeTokens(tokens, refreshed);
              setServiceUser(refreshed);
            } catch {
              authClient.clearStoredSession();
              setServiceUser(null);
            }
          })
          .finally(() => setIsLoading(false));
      } else {
        setIsLoading(false);
      }
    } else {
      // Admin key mode — restore from sessionStorage
      const stored = sessionStorage.getItem(ADMIN_KEY_STORAGE);
      if (stored) setAdminKey(stored);
      setIsLoading(false);
    }
  }, []);

  // ── Login ──────────────────────────────────────────────────────────────────────
  const login = useCallback(async (emailOrKey: string, password?: string) => {
    if (USE_REAL_AUTH) {
      if (!password) throw new Error('Password required in JWT mode');
      const { user, tokens } = await authClient.login(emailOrKey, password);
      authClient.storeTokens(tokens, user);
      setServiceUser(user);
    } else {
      // Admin key mode: validate the key by hitting /health
      const baseUrl = import.meta.env.VITE_API_URL ?? '';
      const res = await fetch(`${baseUrl}/health`, {
        headers: { Authorization: `Bearer ${emailOrKey}` }
      });
      if (!res.ok) {
        throw new Error('Chave de administrador inválida ou gateway indisponível.');
      }
      sessionStorage.setItem(ADMIN_KEY_STORAGE, emailOrKey);
      setAdminKey(emailOrKey);
    }
  }, []);

  // ── Logout ─────────────────────────────────────────────────────────────────────
  const logout = useCallback(() => {
    if (USE_REAL_AUTH) {
      const session = authClient.loadStoredSession();
      if (session) authClient.logout(session.accessToken).catch(() => { /* best-effort */ });
      authClient.clearStoredSession();
      setServiceUser(null);
    } else {
      sessionStorage.removeItem(ADMIN_KEY_STORAGE);
      setAdminKey(null);
    }
    window.location.replace('/login');
  }, []);

  const isAuthenticated = USE_REAL_AUTH ? !!serviceUser : !!adminKey;
  const isAdmin = USE_REAL_AUTH
    ? (serviceUser?.role === 'owner' || serviceUser?.role === 'admin')
    : !!adminKey; // admin-key holders have full access by definition

  const value = useMemo<AuthContextValue>(
    () => ({ adminKey, serviceUser, isAuthenticated, isLoading, login, logout, isAdmin }),
    [adminKey, serviceUser, isAuthenticated, isLoading, login, logout, isAdmin]
  );

  return <AuthContext.Provider value={value}>{children}</AuthContext.Provider>;
}

export function useAuth(): AuthContextValue {
  const ctx = useContext(AuthContext);
  if (!ctx) throw new Error('useAuth must be used within AuthProvider');
  return ctx;
}
