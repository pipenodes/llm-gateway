// db.ts — Base de dados em memória para o MSW
// Simula um backend real com suporte a CRUD e paginação

import type { Role } from 'lib/mockAuth';

export interface UserRecord {
  id: string;
  name: string;
  email: string;
  role: Role;
  status: 'active' | 'inactive' | 'pending';
  department: string;
  createdAt: string;
  lastLogin: string | null;
  avatarUrl?: string;
}

export interface NotificationRecord {
  id: string;
  message: string;
  type: 'info' | 'success' | 'warning' | 'error';
  read: boolean;
  timestamp: string;
  userId?: string;
}

// ── Usuários mock (20 registros) ─────────────────────────────────────────────
const INITIAL_USERS: UserRecord[] = [
  { id: 'usr-001', name: 'Ana Administradora', email: 'admin@empresa.com', role: 'admin', status: 'active', department: 'TI', createdAt: '2024-01-10', lastLogin: '2026-02-20 01:47', avatarUrl: 'https://api.dicebear.com/7.x/avataaars/svg?seed=Ana' },
  { id: 'usr-002', name: 'Bruno Gerente', email: 'manager@empresa.com', role: 'manager', status: 'active', department: 'Operações', createdAt: '2024-02-14', lastLogin: '2026-02-20 01:45', avatarUrl: 'https://api.dicebear.com/7.x/avataaars/svg?seed=Bruno' },
  { id: 'usr-003', name: 'Carla Usuária', email: 'user@empresa.com', role: 'user', status: 'active', department: 'Financeiro', createdAt: '2024-03-05', lastLogin: '2026-02-19 14:30', avatarUrl: 'https://api.dicebear.com/7.x/avataaars/svg?seed=Carla' },
  { id: 'usr-004', name: 'Diego Visitante', email: 'guest@empresa.com', role: 'guest', status: 'active', department: 'Parceiros', createdAt: '2024-04-20', lastLogin: '2026-02-18 09:00', avatarUrl: 'https://api.dicebear.com/7.x/avataaars/svg?seed=Diego' },
  { id: 'usr-005', name: 'Elena Ferreira', email: 'elena@empresa.com', role: 'manager', status: 'active', department: 'RH', createdAt: '2024-05-11', lastLogin: '2026-02-20 08:15', avatarUrl: 'https://api.dicebear.com/7.x/avataaars/svg?seed=Elena' },
  { id: 'usr-006', name: 'Felipe Santos', email: 'felipe@empresa.com', role: 'user', status: 'active', department: 'Engenharia', createdAt: '2024-06-02', lastLogin: '2026-02-19 16:45', avatarUrl: 'https://api.dicebear.com/7.x/avataaars/svg?seed=Felipe' },
  { id: 'usr-007', name: 'Gabriela Lima', email: 'gabriela@empresa.com', role: 'user', status: 'inactive', department: 'Marketing', createdAt: '2024-06-15', lastLogin: '2026-01-30 11:00', avatarUrl: 'https://api.dicebear.com/7.x/avataaars/svg?seed=Gabriela' },
  { id: 'usr-008', name: 'Henrique Oliveira', email: 'henrique@empresa.com', role: 'admin', status: 'active', department: 'TI', createdAt: '2024-07-01', lastLogin: '2026-02-20 00:30', avatarUrl: 'https://api.dicebear.com/7.x/avataaars/svg?seed=Henrique' },
  { id: 'usr-009', name: 'Isabela Costa', email: 'isabela@empresa.com', role: 'user', status: 'pending', department: 'Vendas', createdAt: '2026-02-10', lastLogin: null, avatarUrl: 'https://api.dicebear.com/7.x/avataaars/svg?seed=Isabela' },
  { id: 'usr-010', name: 'João Pereira', email: 'joao@empresa.com', role: 'user', status: 'active', department: 'Suporte', createdAt: '2024-08-22', lastLogin: '2026-02-17 13:20', avatarUrl: 'https://api.dicebear.com/7.x/avataaars/svg?seed=Joao' },
  { id: 'usr-011', name: 'Karen Moreira', email: 'karen@empresa.com', role: 'manager', status: 'active', department: 'Produto', createdAt: '2024-09-08', lastLogin: '2026-02-19 10:05', avatarUrl: 'https://api.dicebear.com/7.x/avataaars/svg?seed=Karen' },
  { id: 'usr-012', name: 'Lucas Araujo', email: 'lucas@empresa.com', role: 'user', status: 'active', department: 'Engenharia', createdAt: '2024-09-30', lastLogin: '2026-02-18 17:50', avatarUrl: 'https://api.dicebear.com/7.x/avataaars/svg?seed=Lucas' },
  { id: 'usr-013', name: 'Marina Rodrigues', email: 'marina@empresa.com', role: 'user', status: 'inactive', department: 'Financeiro', createdAt: '2024-10-14', lastLogin: '2026-01-15 09:30', avatarUrl: 'https://api.dicebear.com/7.x/avataaars/svg?seed=Marina' },
  { id: 'usr-014', name: 'Nelson Carvalho', email: 'nelson@empresa.com', role: 'guest', status: 'active', department: 'Parceiros', createdAt: '2024-11-05', lastLogin: '2026-02-16 14:00', avatarUrl: 'https://api.dicebear.com/7.x/avataaars/svg?seed=Nelson' },
  { id: 'usr-015', name: 'Olivia Batista', email: 'olivia@empresa.com', role: 'user', status: 'active', department: 'Marketing', createdAt: '2024-11-20', lastLogin: '2026-02-20 07:45', avatarUrl: 'https://api.dicebear.com/7.x/avataaars/svg?seed=Olivia' },
  { id: 'usr-016', name: 'Paulo Mendes', email: 'paulo@empresa.com', role: 'user', status: 'active', department: 'Vendas', createdAt: '2024-12-01', lastLogin: '2026-02-19 15:30', avatarUrl: 'https://api.dicebear.com/7.x/avataaars/svg?seed=Paulo' },
  { id: 'usr-017', name: 'Quésia Alves', email: 'quesia@empresa.com', role: 'user', status: 'pending', department: 'RH', createdAt: '2026-01-15', lastLogin: null, avatarUrl: 'https://api.dicebear.com/7.x/avataaars/svg?seed=Quesia' },
  { id: 'usr-018', name: 'Rafael Sousa', email: 'rafael@empresa.com', role: 'manager', status: 'active', department: 'Operações', createdAt: '2025-01-10', lastLogin: '2026-02-20 06:00', avatarUrl: 'https://api.dicebear.com/7.x/avataaars/svg?seed=Rafael' },
  { id: 'usr-019', name: 'Sabrina Torres', email: 'sabrina@empresa.com', role: 'user', status: 'active', department: 'Suporte', createdAt: '2025-02-20', lastLogin: '2026-02-18 12:15', avatarUrl: 'https://api.dicebear.com/7.x/avataaars/svg?seed=Sabrina' },
  { id: 'usr-020', name: 'Thiago Vieira', email: 'thiago@empresa.com', role: 'user', status: 'active', department: 'Produto', createdAt: '2025-03-05', lastLogin: '2026-02-17 11:00', avatarUrl: 'https://api.dicebear.com/7.x/avataaars/svg?seed=Thiago' },
];

const INITIAL_NOTIFICATIONS: NotificationRecord[] = [
  { id: 'ntf-001', message: 'Isabela Costa criou uma nova conta e aguarda aprovação.', type: 'info', read: false, timestamp: '2026-02-20 09:00', userId: 'usr-009' },
  { id: 'ntf-002', message: 'Quésia Alves criou uma nova conta e aguarda aprovação.', type: 'info', read: false, timestamp: '2026-02-20 08:30', userId: 'usr-017' },
  { id: 'ntf-003', message: 'Backup semanal concluído com sucesso.', type: 'success', read: false, timestamp: '2026-02-20 04:00' },
  { id: 'ntf-004', message: 'Carla Usuária tentou acessar recurso sem permissão.', type: 'warning', read: true, timestamp: '2026-02-20 01:43', userId: 'usr-003' },
  { id: 'ntf-005', message: 'Taxa de erros da API subiu acima de 2% por 15 minutos.', type: 'error', read: true, timestamp: '2026-02-19 23:15' },
  { id: 'ntf-006', message: '7 dependências com atualizações de segurança disponíveis.', type: 'warning', read: true, timestamp: '2026-02-19 08:00' },
];

// ── Estado em memória (simula banco de dados) ────────────────────────────────
let users: UserRecord[] = [...INITIAL_USERS];
let notifications: NotificationRecord[] = [...INITIAL_NOTIFICATIONS];
let nextUserId = 21;

export const db = {
  // Users
  getUsers: () => [...users],
  getUserById: (id: string) => users.find((u) => u.id === id) ?? null,
  createUser: (data: Omit<UserRecord, 'id' | 'createdAt' | 'lastLogin'>): UserRecord => {
    const newUser: UserRecord = {
      ...data,
      id: `usr-${String(nextUserId++).padStart(3, '0')}`,
      createdAt: new Date().toISOString().slice(0, 10),
      lastLogin: null,
      avatarUrl: `https://api.dicebear.com/7.x/avataaars/svg?seed=${encodeURIComponent(data.name)}`
    };
    users = [...users, newUser];
    return newUser;
  },
  updateUser: (id: string, data: Partial<UserRecord>): UserRecord | null => {
    const idx = users.findIndex((u) => u.id === id);
    if (idx === -1) return null;
    users = users.map((u) => (u.id === id ? { ...u, ...data } : u));
    return users[idx];
  },
  deleteUser: (id: string): boolean => {
    const len = users.length;
    users = users.filter((u) => u.id !== id);
    return users.length < len;
  },
  resetUsers: () => { users = [...INITIAL_USERS]; nextUserId = 21; },

  // Notifications
  getNotifications: () => [...notifications],
  getUnreadCount: () => notifications.filter((n) => !n.read).length,
  markAllRead: () => { notifications = notifications.map((n) => ({ ...n, read: true })); },
  markRead: (id: string) => { notifications = notifications.map((n) => n.id === id ? { ...n, read: true } : n); },
};
