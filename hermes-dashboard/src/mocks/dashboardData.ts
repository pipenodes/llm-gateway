// dashboardData.ts — Dados mock para o Dashboard (template padrão)
// Substitua por chamadas reais ao conectar o backend

export interface KpiCard {
  id: string;
  label: string;
  value: string;
  trend: number;
  trendLabel: string;
  color: 'primary' | 'success' | 'warning' | 'error';
}

export const KPI_CARDS: KpiCard[] = [
  { id: 'users', label: 'Total de Usuários', value: '10', trend: 0, trendLabel: 'vs. mês anterior', color: 'primary' },
  { id: 'revenue', label: 'Receita (mês)', value: 'R$ 0', trend: 0, trendLabel: 'vs. mês anterior', color: 'success' },
  { id: 'requests', label: 'Requisições', value: '0', trend: 0, trendLabel: 'vs. mês anterior', color: 'warning' },
  { id: 'errors', label: 'Taxa de Erros', value: '0%', trend: 0, trendLabel: 'vs. mês anterior', color: 'error' }
];

export const REVENUE_BY_PLAN = [
  { plan: 'Free', revenue: 0, users: 2 },
  { plan: 'Starter', revenue: 1000, users: 3 },
  { plan: 'Pro', revenue: 5000, users: 4 },
  { plan: 'Enterprise', revenue: 15000, users: 1 }
];

export const USER_GROWTH = [
  { month: 'Abr', users: 4 },
  { month: 'Mai', users: 5 },
  { month: 'Jun', users: 5 },
  { month: 'Jul', users: 6 },
  { month: 'Ago', users: 7 },
  { month: 'Set', users: 7 },
  { month: 'Out', users: 8 },
  { month: 'Nov', users: 8 },
  { month: 'Dez', users: 9 },
  { month: 'Jan', users: 9 },
  { month: 'Fev', users: 10 },
  { month: 'Mar', users: 10 }
];

export const USERS_BY_ROLE = [
  { role: 'Administradores do sistema', count: 1, color: '#ff4d4f' },
  { role: 'Administradores', count: 3, color: '#fa8c16' },
  { role: 'Gerentes', count: 2, color: '#faad14' },
  { role: 'Operadores', count: 2, color: '#1677ff' },
  { role: 'Visitantes', count: 2, color: '#8c8c8c' }
];

export interface UsersByEmpresaStackedRow {
  shortName: string;
  empresaId: string;
  admin: number;
  gerente: number;
  operador: number;
  visitante: number;
  [key: string]: string | number;
}

export const ROLE_CHART_COLORS = {
  admin: '#fa8c16',
  gerente: '#faad14',
  operador: '#1677ff',
  visitante: '#8c8c8c'
} as const;

export interface ActivityRow {
  id: string;
  timestamp: string;
  user: string;
  role: string;
  action: string;
  resource: string;
  status: 'success' | 'denied' | 'error';
  ip: string;
}

export const RECENT_ACTIVITY: ActivityRow[] = [
  { id: 'act-01', timestamp: '2025-03-05 10:00:00', user: 'Ana Super Admin', role: 'super_admin', action: 'login', resource: 'auth', status: 'success', ip: '192.168.1.10' },
  { id: 'act-02', timestamp: '2025-03-05 09:55:00', user: 'Bruno Admin', role: 'admin', action: 'report.export', resource: 'reports', status: 'success', ip: '10.0.0.22' },
  { id: 'act-03', timestamp: '2025-03-05 09:50:00', user: 'Carla Gerente', role: 'gerente', action: 'settings.access', resource: 'settings', status: 'denied', ip: '172.16.5.3' },
  { id: 'act-04', timestamp: '2025-03-05 09:45:00', user: 'Diego Operador', role: 'operador', action: 'dashboard.view', resource: 'dashboard', status: 'success', ip: '10.0.0.45' },
  { id: 'act-05', timestamp: '2025-03-05 09:40:00', user: 'Elena Visitante', role: 'visitante', action: 'profile.update', resource: 'profile', status: 'success', ip: '10.0.0.50' }
];
