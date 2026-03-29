// Marketplace API client for hermes-dashboard.
// Reads VITE_MARKETPLACE_URL from env (default: http://localhost:8102).

const BASE = import.meta.env.VITE_MARKETPLACE_URL ?? 'http://localhost:8102';
const PRODUCT = 'llm-gateway';

function authHeaders(): Record<string, string> {
  const token = localStorage.getItem('access_token');
  return token ? { Authorization: `Bearer ${token}` } : {};
}

async function get<T>(path: string): Promise<T> {
  const res = await fetch(`${BASE}${path}`, { headers: authHeaders() });
  if (!res.ok) throw new Error(`Marketplace ${res.status}: ${res.statusText}`);
  return res.json();
}

async function post<T>(path: string, body: unknown): Promise<T> {
  const res = await fetch(`${BASE}${path}`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json', ...authHeaders() },
    body: JSON.stringify(body),
  });
  if (!res.ok) throw new Error(`Marketplace ${res.status}: ${res.statusText}`);
  return res.json();
}

export interface MarketplacePlugin {
  name: string;
  version: string;
  product: string;
  tier: 'core' | 'enterprise';
  description: string;
  min_core_version: string;
  max_core_version: string;
  accessible: boolean;
}

export interface InstallResult {
  plugin: string;
  version: string;
  download_url: string;
  checksum_sha256: string;
}

export interface UpdateInfo {
  name: string;
  current_version: string;
  new_version: string;
  download_url: string;
}

export const marketplaceApi = {
  plugins: (tier?: string, core_version?: string) => {
    const params = new URLSearchParams({ product: PRODUCT });
    if (tier) params.set('tier', tier);
    if (core_version) params.set('core_version', core_version);
    return get<{ plugins: MarketplacePlugin[]; total: number }>(`/marketplace/plugins?${params}`);
  },

  install: (name: string, version?: string) =>
    post<InstallResult>(`/marketplace/plugins/${name}/install`, {
      version,
      product: PRODUCT,
    }),

  checkUpdates: (installed: Array<{ name: string; version: string }>, core_version: string) =>
    post<{ updates: UpdateInfo[] }>('/marketplace/updates/check', {
      product: PRODUCT,
      core_version,
      installed,
    }),
};
