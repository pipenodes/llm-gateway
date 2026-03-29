export interface HealthResponse {
  status: string;
  version?: string;
  uptime?: number;
}

export interface MetricsCacheInfo {
  enabled: boolean;
  entries: number;
  hits: number;
  misses: number;
  hit_rate: number;
  evictions: number;
  memory_bytes: number;
}

export interface MetricsResponse {
  uptime_seconds: number;
  requests_total: number;
  requests_active: number;
  memory_rss_kb: number;
  memory_peak_kb: number;
  cache?: MetricsCacheInfo;
  plugins?: Record<string, Record<string, unknown>>;
  total_tokens_used?: number;
  total_cost?: number;
  requests_per_model?: Record<string, number>;
  tokens_per_model?: Record<string, number>;
  cost_per_model?: Record<string, number>;
  avg_latency_ms?: number;
  p50_latency_ms?: number;
  p95_latency_ms?: number;
  p99_latency_ms?: number;
  rate_limiter?: {
    total_requests: number;
    total_limited: number;
    current_window_requests: number;
    config: {
      max_rpm: number;
      window_seconds: number;
      burst_multiplier: number;
    };
  };
}

export interface PluginInfo {
  name: string;
  version: string;
  position: number;
  type: string;
  enabled: boolean;
  stats?: Record<string, unknown>;
}

export interface ApiKey {
  alias: string;
  key_hash: string;
  rate_limit_rpm: number;
  ip_whitelist: string[];
  created_at: string;
}

export interface CreateKeyRequest {
  alias: string;
  rate_limit_rpm?: number;
  ip_whitelist?: string[];
}

export interface CreateKeyResponse {
  alias: string;
  key: string;
  rate_limit_rpm: number;
}

export interface UsageSummary {
  alias: string;
  total_requests: number;
  total_tokens: number;
  total_prompt_tokens: number;
  total_completion_tokens: number;
  by_model: Record<string, ModelUsage>;
  daily: DailyUsage[];
  monthly: MonthlyUsage[];
}

export interface ModelUsage {
  requests: number;
  tokens: number;
  prompt_tokens: number;
  completion_tokens: number;
}

export interface DailyUsage {
  date: string;
  requests: number;
  tokens: number;
}

export interface MonthlyUsage {
  month: string;
  requests: number;
  tokens: number;
}

export interface CostSummary {
  alias: string;
  total_cost: number;
  budget_limit: number;
  budget_remaining: number;
  budget_used_pct: number;
  by_model: Record<string, ModelCost>;
}

export interface ModelCost {
  requests: number;
  tokens: number;
  cost: number;
}

export interface SetBudgetRequest {
  budget_limit: number;
}

export interface AuditEntry {
  timestamp: string;
  request_id: string;
  key_alias: string;
  method: string;
  path: string;
  model: string;
  status: number;
  latency_ms: number;
  prompt_tokens: number;
  completion_tokens: number;
  total_tokens: number;
  error?: string;
  ip?: string;
}

export interface AuditResponse {
  entries: AuditEntry[];
  total: number;
  offset: number;
  limit: number;
}

export interface PromptTemplate {
  name: string;
  content: string;
  variables: string[];
  description?: string;
}

export interface Guardrail {
  pattern: string;
  action: 'block' | 'warn' | 'redact';
  message: string;
}

export interface ABExperiment {
  name: string;
  trigger: string;
  variants: ABVariant[];
  total_requests: number;
  created_at?: string;
}

export interface ABVariant {
  name: string;
  model: string;
  weight: number;
  requests: number;
  avg_latency_ms: number;
  p50_latency_ms: number;
  p95_latency_ms: number;
  p99_latency_ms: number;
  error_rate: number;
  avg_tokens: number;
}

export interface ABCreateRequest {
  name: string;
  trigger: string;
  variants: { name: string; model: string; weight: number }[];
}

export interface Session {
  session_id: string;
  key_alias: string;
  message_count: number;
  created_at: string;
  last_activity: string;
  messages: SessionMessage[];
}

export interface SessionMessage {
  role: 'system' | 'user' | 'assistant';
  content: string;
  timestamp?: string;
}

export interface WebhookConfig {
  url: string;
  events: string[];
  active: boolean;
}

export interface SecurityPosture {
  score: number;
  checks: SecurityCheck[];
}

export interface SecurityCheck {
  name: string;
  status: 'pass' | 'warn' | 'fail';
  description: string;
  recommendation?: string;
}

export interface ComplianceReport {
  generated_at: string;
  checks: ComplianceCheck[];
  overall_status: 'compliant' | 'non_compliant' | 'partial';
}

export interface ComplianceCheck {
  category: string;
  name: string;
  status: 'pass' | 'fail' | 'warning';
  details: string;
}

export interface GatewayConfig {
  [key: string]: unknown;
}

export interface QueueStatus {
  enabled: boolean;
  active: number;
  max_concurrency: number;
  max_size: number;
  metrics?: {
    active: number;
    avg_wait_ms: number;
    max_concurrency: number;
    max_size: number;
    pending: number;
    total_enqueued: number;
    total_processed: number;
    total_rejected: number;
    total_timed_out: number;
  };
}

export interface ModelInfo {
  id: string;
  object: string;
  created: number;
  owned_by: string;
  root: string;
  parent: string | null;
  permission: unknown[];
}

export interface UpdateCheck {
  core: {
    current: string;
    available: boolean;
    latest?: string;
  };
  error?: string;
  message?: string;
}

// ── Enterprise Security Pipeline types (RF-32 to RF-35) ─────────────────────

export interface GuardrailsStats {
  total_requests: number;
  l1_blocked: number;
  l2_blocked: number;
  tenant_policies: number;
  active_rate_buckets: number;
}

export interface DiscoveryCatalogEntry {
  hash: string;
  tag: string;
  tenant_id: string;
  app_id: string;
  occurrences: number;
  first_seen: number;
  last_seen: number;
}

export interface ShadowAIEvent {
  tenant_id: string;
  app_id: string;
  model: string;
  endpoint: string;
  detected_at: number;
}

export interface DLPQuarantineEntry {
  request_id: string;
  tenant_id: string;
  app_id: string;
  client_id: string;
  tag: string;
  timestamp: number;
  reason: string;
}

export interface FinOpsCostNode {
  requests: number;
  input_tokens: number;
  output_tokens: number;
  cost_usd: number;
  by_model: Record<string, number>;
}

export interface FinOpsBudgetEntry {
  monthly_limit_usd: number;
  daily_limit_usd: number;
  spent_monthly_usd: number;
  spent_daily_usd: number;
  used_pct?: number;
}

export interface SetFinOpsBudgetRequest {
  monthly_limit_usd: number;
  daily_limit_usd: number;
}
