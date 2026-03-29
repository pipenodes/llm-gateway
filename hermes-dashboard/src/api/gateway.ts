import useSWR from 'swr';
import { fetcher, apiFetch } from 'lib/fetcher';
import type {
  MetricsResponse,
  HealthResponse,
  ApiKey,
  CreateKeyRequest,
  CreateKeyResponse,
  UsageSummary,
  CostSummary,
  SetBudgetRequest,
  AuditResponse,
  PluginInfo,
  PromptTemplate,
  Guardrail,
  ABExperiment,
  ABCreateRequest,
  Session,
  WebhookConfig,
  SecurityPosture,
  ComplianceReport,
  GatewayConfig,
  QueueStatus,
  ModelInfo,
  UpdateCheck,
  GuardrailsStats,
  DiscoveryCatalogEntry,
  ShadowAIEvent,
  DLPQuarantineEntry,
  FinOpsCostNode,
  FinOpsBudgetEntry,
  SetFinOpsBudgetRequest
} from 'types/gateway';

export function useHealth() {
  return useSWR<HealthResponse>('/health', { refreshInterval: 10000 });
}

export function useMetrics() {
  return useSWR<MetricsResponse>('/metrics', { refreshInterval: 5000 });
}

const keysFetcher = async (url: string) => {
  const res = await fetcher<{ keys: ApiKey[] }>(url);
  return res.keys ?? [];
};

export function useKeys() {
  const result = useSWR<ApiKey[]>('/admin/keys', keysFetcher);
  return {
    ...result,
    createKey: async (req: CreateKeyRequest) => {
      const res = await apiFetch<CreateKeyResponse>('/admin/keys', { method: 'POST', body: req });
      await result.mutate();
      return res;
    },
    deleteKey: async (alias: string) => {
      await apiFetch(`/admin/keys/${encodeURIComponent(alias)}`, { method: 'DELETE' });
      await result.mutate();
    }
  };
}

export function useUsage() {
  return useSWR<UsageSummary[]>('/admin/usage');
}

export function useUsageByKey(alias: string | null) {
  return useSWR<UsageSummary>(alias ? `/admin/usage/${encodeURIComponent(alias)}` : null);
}

export function useCosts() {
  return useSWR<CostSummary[]>('/admin/costs');
}

export function useCostByKey(alias: string | null) {
  const result = useSWR<CostSummary>(alias ? `/admin/costs/${encodeURIComponent(alias)}` : null);
  return {
    ...result,
    setBudget: async (budget: SetBudgetRequest) => {
      if (!alias) return;
      await apiFetch(`/admin/costs/${encodeURIComponent(alias)}/budget`, { method: 'PUT', body: budget });
      await result.mutate();
    }
  };
}

export function useAudit(limit = 50, offset = 0) {
  return useSWR<AuditResponse>(`/admin/audit?limit=${limit}&offset=${offset}`);
}

const pluginsFetcher = async (url: string) => {
  const res = await fetcher<{ plugins: PluginInfo[] }>(url);
  return res.plugins ?? [];
};

export function usePlugins() {
  return useSWR<PluginInfo[]>('/admin/plugins', pluginsFetcher);
}

export function usePromptTemplates() {
  const result = useSWR<PromptTemplate[]>('/admin/prompts/templates');
  return {
    ...result,
    createTemplate: async (tpl: PromptTemplate) => {
      await apiFetch('/admin/prompts/templates', { method: 'POST', body: tpl });
      await result.mutate();
    }
  };
}

export function useGuardrails() {
  return useSWR<Guardrail[]>('/admin/prompts/guardrails');
}

export function useABExperiments() {
  const result = useSWR<ABExperiment[]>('/admin/ab');
  return {
    ...result,
    createExperiment: async (req: ABCreateRequest) => {
      await apiFetch('/admin/ab', { method: 'POST', body: req });
      await result.mutate();
    },
    deleteExperiment: async (name: string) => {
      await apiFetch(`/admin/ab/${encodeURIComponent(name)}`, { method: 'DELETE' });
      await result.mutate();
    }
  };
}

export function useABExperiment(name: string | null) {
  return useSWR<ABExperiment>(name ? `/admin/ab/${encodeURIComponent(name)}` : null);
}

export function useSessions() {
  return useSWR<Session[]>('/v1/sessions');
}

export function useSession(id: string | null) {
  return useSWR<Session>(id ? `/v1/sessions/${encodeURIComponent(id)}` : null);
}

export async function deleteSession(id: string) {
  await apiFetch(`/v1/sessions/${encodeURIComponent(id)}`, { method: 'DELETE' });
}

export function useWebhooks() {
  return useSWR<WebhookConfig[]>('/admin/webhooks');
}

export async function testWebhook() {
  return apiFetch<{ status: string }>('/admin/webhooks/test', { method: 'POST' });
}

export function useSecurityPosture() {
  return useSWR<SecurityPosture>('/admin/posture');
}

export function useComplianceReport() {
  return useSWR<ComplianceReport>('/admin/compliance/report');
}

export async function runSecurityScan() {
  return apiFetch<SecurityPosture>('/admin/security-scan', { method: 'POST' });
}

export function useGatewayConfig() {
  return useSWR<GatewayConfig>('/admin/config');
}

export function useQueueStatus() {
  const result = useSWR<QueueStatus>('/admin/queue');
  return {
    ...result,
    clearQueue: async () => {
      await apiFetch('/admin/queue', { method: 'DELETE' });
      await result.mutate();
    }
  };
}

const modelsFetcher = async (url: string) => {
  const res = await fetcher<{ data: ModelInfo[] }>(url);
  return res.data ?? [];
};

export function useModels() {
  return useSWR<ModelInfo[]>('/v1/models', modelsFetcher);
}

export function useUpdateCheck() {
  return useSWR<UpdateCheck>('/admin/updates/check', { revalidateOnFocus: false });
}

// ── Enterprise Security Pipeline hooks (RF-32 to RF-35) ──────────────────────

export function useGuardrailsStats() {
  return useSWR<GuardrailsStats>('/admin/guardrails/stats', { refreshInterval: 15000 });
}

export function useDiscoveryCatalog(tenantId: string) {
  return useSWR<DiscoveryCatalogEntry[]>(
    tenantId ? `/admin/discovery/catalog/${encodeURIComponent(tenantId)}` : null
  );
}

export function useDiscoveryShadowAI(tenantId: string) {
  return useSWR<ShadowAIEvent[]>(
    tenantId ? `/admin/discovery/shadow-ai/${encodeURIComponent(tenantId)}` : null
  );
}

export function useDLPQuarantine(tenantId: string) {
  return useSWR<DLPQuarantineEntry[]>(
    tenantId ? `/admin/dlp/quarantine/${encodeURIComponent(tenantId)}` : null
  );
}

export function useFinOpsCosts(tenantId: string) {
  return useSWR<Record<string, FinOpsCostNode>>(
    tenantId ? `/admin/finops/costs/${encodeURIComponent(tenantId)}` : null,
    { refreshInterval: 30000 }
  );
}

export function useFinOpsBudgets(tenantId: string) {
  const result = useSWR<Record<string, FinOpsBudgetEntry>>(
    tenantId ? `/admin/finops/budgets/${encodeURIComponent(tenantId)}` : null
  );
  return {
    ...result,
    setBudget: async (req: SetFinOpsBudgetRequest) => {
      if (!tenantId) return;
      await apiFetch(`/admin/finops/budgets/${encodeURIComponent(tenantId)}`, {
        method: 'PUT',
        body: req
      });
      await result.mutate();
    }
  };
}

export async function downloadFinOpsExport() {
  const baseUrl = import.meta.env.VITE_API_URL ?? '';
  const key = sessionStorage.getItem('hermes_admin_key');
  const res = await fetch(`${baseUrl}/admin/finops/export`, {
    headers: {
      Accept: 'text/csv',
      ...(key ? { Authorization: `Bearer ${key}` } : {})
    }
  });
  if (!res.ok) throw new Error(`Export falhou: ${res.status}`);
  const blob = await res.blob();
  const url  = URL.createObjectURL(blob);
  const a    = document.createElement('a');
  a.href     = url;
  a.download = 'finops-export.csv';
  a.click();
  URL.revokeObjectURL(url);
}
