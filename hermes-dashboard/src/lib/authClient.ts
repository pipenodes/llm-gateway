/**
 * authClient.ts — Real auth client for hermes-dashboard.
 *
 * HERMES supports two authentication modes:
 *   1. Admin key mode (default, OSS): bearer key validated against /health
 *   2. JWT mode (cloud/enterprise): uses the unified Pipeleap auth service
 *
 * Mode is selected by VITE_AUTH_URL:
 *   - Not set → admin key mode
 *   - Set     → JWT mode against the auth service
 */

const AUTH_BASE = (): string => {
  const url = import.meta.env.VITE_AUTH_URL as string | undefined;
  if (!url) throw new Error('VITE_AUTH_URL is not set');
  return url.replace(/\/$/, '');
};

export type AuthServiceRole = 'owner' | 'admin' | 'member' | 'viewer';

export interface AuthServiceUser {
  id: string;
  name: string;
  email: string;
  role: AuthServiceRole;
  org_id: string | null;
  avatar_url?: string;
  created_at: string;
}

export interface AuthServiceTokens {
  access_token: string;
  refresh_token: string;
  expires_in: number;
}

export interface LoginResponse {
  user: AuthServiceUser;
  tokens: AuthServiceTokens;
}

async function handleResponse<T>(res: Response): Promise<T> {
  if (!res.ok) {
    let message = `HTTP ${res.status}`;
    try {
      const body = await res.json();
      message = body.error ?? body.message ?? message;
    } catch { /* ignore */ }
    throw new Error(message);
  }
  return res.json() as Promise<T>;
}

export async function login(email: string, password: string): Promise<LoginResponse> {
  const res = await fetch(`${AUTH_BASE()}/auth/login`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ email, password })
  });
  return handleResponse<LoginResponse>(res);
}

export async function me(accessToken: string): Promise<AuthServiceUser> {
  const res = await fetch(`${AUTH_BASE()}/auth/me`, {
    headers: { Authorization: `Bearer ${accessToken}` }
  });
  return handleResponse<AuthServiceUser>(res);
}

export async function logout(accessToken: string): Promise<void> {
  await fetch(`${AUTH_BASE()}/auth/logout`, {
    method: 'POST',
    headers: { Authorization: `Bearer ${accessToken}` }
  });
}

export async function refreshTokens(refreshToken: string): Promise<AuthServiceTokens> {
  const res = await fetch(`${AUTH_BASE()}/auth/refresh`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ refresh_token: refreshToken })
  });
  return handleResponse<AuthServiceTokens>(res);
}

const KEYS = {
  ACCESS: 'pipeleap_access_token',
  REFRESH: 'pipeleap_refresh_token',
  USER: 'pipeleap_user'
} as const;

export function storeTokens(tokens: AuthServiceTokens, user: AuthServiceUser): void {
  localStorage.setItem(KEYS.ACCESS, tokens.access_token);
  localStorage.setItem(KEYS.REFRESH, tokens.refresh_token);
  localStorage.setItem(KEYS.USER, JSON.stringify(user));
}

export function loadStoredSession(): { user: AuthServiceUser; accessToken: string; refreshToken: string } | null {
  const access = localStorage.getItem(KEYS.ACCESS);
  const refresh = localStorage.getItem(KEYS.REFRESH);
  const rawUser = localStorage.getItem(KEYS.USER);
  if (!access || !refresh || !rawUser) return null;
  try {
    return { user: JSON.parse(rawUser) as AuthServiceUser, accessToken: access, refreshToken: refresh };
  } catch {
    return null;
  }
}

export function clearStoredSession(): void {
  Object.values(KEYS).forEach((k) => localStorage.removeItem(k));
}
