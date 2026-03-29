// mockAuth.ts — Usuários e permissões mock (RBAC por empresa + super_admin)
// Substitua por integração real com o backend em produção

/** Papel por empresa (admin = gerente da empresa) */
export type RoleEmpresa = 'admin' | 'gerente' | 'operador' | 'visitante';

/** Role efetivo na UI: super_admin é global; demais são por empresa */
export type Role = 'super_admin' | RoleEmpresa;

export interface EmpresaMembership {
  empresaId: string;
  role: RoleEmpresa;
  status: 'ativo' | 'convidado' | 'bloqueado' | 'aguardando_aprovacao';
}

export interface MockUser {
  id: string;
  name: string;
  email: string;
  password: string;
  avatarUrl?: string;
  /** True apenas para administrador do sistema (vê todas as empresas, pode impersonar qualquer um) */
  isSuperAdmin: boolean;
  /** Vínculos por empresa; vazio se isSuperAdmin (acesso a todas sem precisar de entrada) */
  empresas: EmpresaMembership[];
}

export interface AuthToken {
  userId: string;
  exp: number;
  isSuperAdmin?: boolean;
  /** @deprecated use user from MOCK_USERS */
  role?: string;
}

// ── Usuários mock: super_admin + múltiplos por empresa com papéis distintos ─
export const MOCK_USERS: MockUser[] = [
  {
    id: 'usr-001',
    name: 'Ana Super Admin',
    email: 'superadmin@empresa.com',
    password: 'super123',
    isSuperAdmin: true,
    empresas: [],
    avatarUrl: 'https://api.dicebear.com/7.x/avataaars/svg?seed=Ana&backgroundColor=b6e3f4'
  },
  {
    id: 'usr-002',
    name: 'Bruno Admin Alpha',
    email: 'admin@alpha.com',
    password: 'admin123',
    isSuperAdmin: false,
    empresas: [
      { empresaId: 'emp-001', role: 'admin', status: 'ativo' },
      { empresaId: 'emp-002', role: 'gerente', status: 'ativo' }
    ],
    avatarUrl: 'https://api.dicebear.com/7.x/avataaars/svg?seed=Bruno&backgroundColor=c0aede'
  },
  {
    id: 'usr-003',
    name: 'Carla Gerente',
    email: 'gerente@empresa.com',
    password: 'gerente123',
    isSuperAdmin: false,
    empresas: [
      { empresaId: 'emp-001', role: 'gerente', status: 'ativo' },
      { empresaId: 'emp-003', role: 'gerente', status: 'ativo' }
    ],
    avatarUrl: 'https://api.dicebear.com/7.x/avataaars/svg?seed=Carla&backgroundColor=d1d4f9'
  },
  {
    id: 'usr-004',
    name: 'Diego Operador',
    email: 'operador@empresa.com',
    password: 'operador123',
    isSuperAdmin: false,
    empresas: [
      { empresaId: 'emp-001', role: 'operador', status: 'ativo' },
      { empresaId: 'emp-002', role: 'operador', status: 'ativo' }
    ],
    avatarUrl: 'https://api.dicebear.com/7.x/avataaars/svg?seed=Diego&backgroundColor=ffd5dc'
  },
  {
    id: 'usr-005',
    name: 'Elena Visitante',
    email: 'visitante@empresa.com',
    password: 'visitante123',
    isSuperAdmin: false,
    empresas: [
      { empresaId: 'emp-001', role: 'visitante', status: 'ativo' },
      { empresaId: 'emp-003', role: 'visitante', status: 'ativo' }
    ],
    avatarUrl: 'https://api.dicebear.com/7.x/avataaars/svg?seed=Elena&backgroundColor=b6e3f4'
  },
  {
    id: 'usr-006',
    name: 'Felipe Admin Beta',
    email: 'admin@beta.com',
    password: 'admin123',
    isSuperAdmin: false,
    empresas: [
      { empresaId: 'emp-002', role: 'admin', status: 'ativo' },
      { empresaId: 'emp-004', role: 'gerente', status: 'ativo' }
    ],
    avatarUrl: 'https://api.dicebear.com/7.x/avataaars/svg?seed=Felipe&backgroundColor=c0aede'
  },
  {
    id: 'usr-007',
    name: 'Gabriela Operadora',
    email: 'gabriela@empresa.com',
    password: 'user123',
    isSuperAdmin: false,
    empresas: [{ empresaId: 'emp-003', role: 'operador', status: 'ativo' }],
    avatarUrl: 'https://api.dicebear.com/7.x/avataaars/svg?seed=Gabriela&backgroundColor=d1d4f9'
  },
  {
    id: 'usr-008',
    name: 'Henrique Admin Delta',
    email: 'henrique@delta.com',
    password: 'admin123',
    isSuperAdmin: false,
    empresas: [{ empresaId: 'emp-004', role: 'admin', status: 'ativo' }],
    avatarUrl: 'https://api.dicebear.com/7.x/avataaars/svg?seed=Henrique&backgroundColor=ffd5dc'
  },
  {
    id: 'usr-009',
    name: 'Isabela Visitante',
    email: 'isabela@empresa.com',
    password: 'user123',
    isSuperAdmin: false,
    empresas: [{ empresaId: 'emp-005', role: 'visitante', status: 'ativo' }],
    avatarUrl: 'https://api.dicebear.com/7.x/avataaars/svg?seed=Isabela&backgroundColor=b6e3f4'
  },
  {
    id: 'usr-010',
    name: 'João Gerente',
    email: 'joao@empresa.com',
    password: 'gerente123',
    isSuperAdmin: false,
    empresas: [{ empresaId: 'emp-005', role: 'gerente', status: 'ativo' }],
    avatarUrl: 'https://api.dicebear.com/7.x/avataaars/svg?seed=Joao&backgroundColor=c0aede'
  }
];

// ── Permissões por role (RBAC) ─────────────────────────────────────────────
// '*' = acesso total
export const ROLE_PERMISSIONS: Record<Role, string[]> = {
  super_admin: ['*'],
  admin: [
    'dashboard:read',
    'empresas:read',
    'empresas:write',
    'users:read',
    'users:write',
    'reports:read',
    'profile:read',
    'profile:write',
    'notifications:read'
  ],
  gerente: [
    'dashboard:read',
    'empresas:read',
    'users:read',
    'users:write',
    'reports:read',
    'profile:read',
    'profile:write',
    'notifications:read'
  ],
  operador: [
    'dashboard:read',
    'profile:read',
    'profile:write',
    'notifications:read'
  ],
  visitante: ['dashboard:read', 'profile:read', 'notifications:read']
};

export const ROLE_LABELS: Record<Role, string> = {
  super_admin: 'Administrador do sistema',
  admin: 'Administrador',
  gerente: 'Gerente',
  operador: 'Operador',
  visitante: 'Visitante'
};

export function getMockUserById(userId: string): MockUser | undefined {
  return MOCK_USERS.find((u) => u.id === userId);
}

/** Retorna empresas em que o usuário tem vínculo ativo (ou todas se super_admin) */
export function getEmpresaIdsForUser(user: MockUser): string[] {
  if (user.isSuperAdmin) return [];
  return user.empresas.filter((e) => e.status === 'ativo').map((e) => e.empresaId);
}

/** Role efetivo na empresa ativa: super_admin ou role na empresa */
export function getEffectiveRole(user: MockUser, empresaId: string | null): Role {
  if (user.isSuperAdmin) return 'super_admin';
  if (!empresaId) return 'visitante';
  const m = user.empresas.find((e) => e.empresaId === empresaId && e.status === 'ativo');
  return m ? m.role : 'visitante';
}

/** Verifica se o usuário pode impersonar o alvo. Super_admin: qualquer; admin: só da empresa ativa. */
export function canImpersonate(
  actor: MockUser,
  targetUserId: string,
  empresaAtivaId: string | null
): boolean {
  if (!actor.isSuperAdmin) {
    const actorRoleInEmpresa = actor.empresas.find(
      (e) => e.empresaId === empresaAtivaId && e.status === 'ativo'
    );
    if (actorRoleInEmpresa?.role !== 'admin') return false;
    const target = getMockUserById(targetUserId);
    if (!target) return false;
    const targetInEmpresa = target.empresas.some(
      (e) => e.empresaId === empresaAtivaId && e.status === 'ativo'
    );
    return targetInEmpresa;
  }
  return true;
}

// ── Login e token ──────────────────────────────────────────────────────────
export async function mockLogin(
  email: string,
  password: string
): Promise<{ user: MockUser; token: string }> {
  await new Promise((r) => setTimeout(r, 600));

  const found = MOCK_USERS.find(
    (u) => u.email.toLowerCase() === email.toLowerCase() && u.password === password
  );

  if (!found) {
    throw new Error('E-mail ou senha incorretos.');
  }

  const payload: AuthToken = {
    userId: found.id,
    isSuperAdmin: found.isSuperAdmin,
    exp: Date.now() + 8 * 60 * 60 * 1000
  };
  const token = btoa(JSON.stringify(payload));

  return { user: found, token };
}

export function decodeToken(token: string): AuthToken | null {
  try {
    return JSON.parse(atob(token)) as AuthToken;
  } catch {
    return null;
  }
}

export function isTokenExpired(token: string): boolean {
  const decoded = decodeToken(token);
  if (!decoded) return true;
  return Date.now() > decoded.exp;
}
