import { describe, it, expect } from 'vitest';
import {
  mockLogin,
  decodeToken,
  isTokenExpired,
  MOCK_USERS,
  ROLE_PERMISSIONS,
  ROLE_LABELS,
  getEffectiveRole
} from './mockAuth';

describe('mockAuth', () => {
  describe('mockLogin', () => {
    it('retorna usuário e token para credenciais válidas', async () => {
      const result = await mockLogin('admin@alpha.com', 'admin123');
      expect(result.user.email).toBe('admin@alpha.com');
      expect(result.user.isSuperAdmin).toBe(false);
      expect(result.user.empresas.length).toBeGreaterThan(0);
      expect(result.token).toBeTruthy();
    });

    it('lança erro para credenciais inválidas', async () => {
      await expect(mockLogin('invalido@empresa.com', 'wrong')).rejects.toThrow('E-mail ou senha incorretos');
    });

    it('lança erro para senha incorreta', async () => {
      await expect(mockLogin('admin@alpha.com', 'senhaerrada')).rejects.toThrow();
    });

    it('retorna token decodificável com userId e isSuperAdmin', async () => {
      const { user, token } = await mockLogin('gerente@empresa.com', 'gerente123');
      const decoded = decodeToken(token);
      expect(decoded?.userId).toBe(user.id);
      expect(decoded?.isSuperAdmin).toBe(false);
    });

    it('token expira em 8 horas', async () => {
      const { token } = await mockLogin('superadmin@empresa.com', 'super123');
      const decoded = decodeToken(token);
      const diffMs = (decoded!.exp as number) - Date.now();
      expect(diffMs).toBeGreaterThan(7 * 60 * 60 * 1000);
      expect(diffMs).toBeLessThan(9 * 60 * 60 * 1000);
    });

    it('login é case-insensitive para o e-mail', async () => {
      const result = await mockLogin('SUPERADMIN@EMPRESA.COM', 'super123');
      expect(result.user.isSuperAdmin).toBe(true);
    });
  });

  describe('decodeToken', () => {
    it('retorna payload para token válido', async () => {
      const { token } = await mockLogin('operador@empresa.com', 'operador123');
      const decoded = decodeToken(token);
      expect(decoded).not.toBeNull();
      expect(decoded?.userId).toBeTruthy();
      expect(decoded?.exp).toBeGreaterThan(Date.now());
    });

    it('retorna null para token inválido', () => {
      expect(decodeToken('nao-e-base64-valido!!!')).toBeNull();
      expect(decodeToken('')).toBeNull();
    });
  });

  describe('isTokenExpired', () => {
    it('retorna false para token válido', async () => {
      const { token } = await mockLogin('visitante@empresa.com', 'visitante123');
      expect(isTokenExpired(token)).toBe(false);
    });

    it('retorna true para token com exp no passado', () => {
      const expired = btoa(JSON.stringify({ userId: 'usr-001', isSuperAdmin: true, exp: Date.now() - 1000 }));
      expect(isTokenExpired(expired)).toBe(true);
    });

    it('retorna true para token inválido', () => {
      expect(isTokenExpired('lixo')).toBe(true);
    });
  });

  describe('MOCK_USERS', () => {
    it('contém super_admin e roles por empresa (admin, gerente, operador, visitante)', () => {
      const superAdmins = MOCK_USERS.filter((u) => u.isSuperAdmin);
      expect(superAdmins.length).toBeGreaterThanOrEqual(1);
      const withEmpresas = MOCK_USERS.filter((u) => !u.isSuperAdmin && u.empresas.length > 0);
      const rolesInData = new Set(withEmpresas.flatMap((u) => u.empresas.map((e) => e.role)));
      expect(rolesInData.has('admin')).toBe(true);
      expect(rolesInData.has('gerente')).toBe(true);
      expect(rolesInData.has('operador')).toBe(true);
      expect(rolesInData.has('visitante')).toBe(true);
    });

    it('todos os usuários têm id, name e email', () => {
      MOCK_USERS.forEach((u) => {
        expect(u.id).toBeTruthy();
        expect(u.name).toBeTruthy();
        expect(u.email).toMatch(/@/);
      });
    });
  });

  describe('getEffectiveRole', () => {
    it('super_admin retorna super_admin para qualquer empresa', () => {
      const superUser = MOCK_USERS.find((u) => u.isSuperAdmin)!;
      expect(getEffectiveRole(superUser, 'emp-001')).toBe('super_admin');
      expect(getEffectiveRole(superUser, null)).toBe('super_admin');
    });

    it('usuário com vínculo na empresa retorna role da empresa', () => {
      const adminUser = MOCK_USERS.find((u) => u.empresas.some((e) => e.empresaId === 'emp-001' && e.role === 'admin'))!;
      expect(getEffectiveRole(adminUser, 'emp-001')).toBe('admin');
    });
  });

  describe('ROLE_PERMISSIONS', () => {
    it('super_admin tem acesso total (*)', () => {
      expect(ROLE_PERMISSIONS.super_admin).toContain('*');
    });

    it('visitante tem dashboard:read e profile:read', () => {
      expect(ROLE_PERMISSIONS.visitante).toContain('dashboard:read');
      expect(ROLE_PERMISSIONS.visitante).toContain('profile:read');
    });

    it('gerente pode ler usuários', () => {
      expect(ROLE_PERMISSIONS.gerente).toContain('users:read');
    });
  });

  describe('ROLE_LABELS', () => {
    it('todos os roles têm label', () => {
      expect(ROLE_LABELS.super_admin).toBe('Administrador do sistema');
      expect(ROLE_LABELS.admin).toBe('Administrador');
      expect(ROLE_LABELS.gerente).toBe('Gerente');
      expect(ROLE_LABELS.operador).toBe('Operador');
      expect(ROLE_LABELS.visitante).toBe('Visitante');
    });
  });
});
