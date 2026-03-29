import { describe, it, expect, vi } from 'vitest';
import { render, screen } from '@testing-library/react';
import RoleGuard from './RoleGuard';

// Mock do useAuth com role configurável
const mockHasRole = vi.fn();

vi.mock('contexts/AuthContext', () => ({
  useAuth: () => ({ hasRole: mockHasRole })
}));

function renderGuard(roles: string[], fallback?: React.ReactNode) {
  return render(
    <RoleGuard roles={roles as never} fallback={fallback}>
      <span>Conteúdo protegido</span>
    </RoleGuard>
  );
}

describe('RoleGuard', () => {
  it('exibe children quando o role é autorizado', () => {
    mockHasRole.mockReturnValue(true);
    renderGuard(['admin']);
    expect(screen.getByText('Conteúdo protegido')).toBeInTheDocument();
  });

  it('oculta children quando o role não é autorizado', () => {
    mockHasRole.mockReturnValue(false);
    renderGuard(['admin']);
    expect(screen.queryByText('Conteúdo protegido')).not.toBeInTheDocument();
  });

  it('exibe fallback quando não autorizado e fallback fornecido', () => {
    mockHasRole.mockReturnValue(false);
    renderGuard(['admin'], <span>Sem permissão</span>);
    expect(screen.getByText('Sem permissão')).toBeInTheDocument();
    expect(screen.queryByText('Conteúdo protegido')).not.toBeInTheDocument();
  });

  it('não exibe fallback quando autorizado', () => {
    mockHasRole.mockReturnValue(true);
    renderGuard(['admin'], <span>Sem permissão</span>);
    expect(screen.getByText('Conteúdo protegido')).toBeInTheDocument();
    expect(screen.queryByText('Sem permissão')).not.toBeInTheDocument();
  });

  it('passa o array de roles para hasRole', () => {
    mockHasRole.mockReturnValue(true);
    renderGuard(['admin', 'manager']);
    expect(mockHasRole).toHaveBeenCalledWith(['admin', 'manager']);
  });

  it('fallback padrão é null (nada renderizado)', () => {
    mockHasRole.mockReturnValue(false);
    const { container } = renderGuard(['admin']);
    expect(container).toBeEmptyDOMElement();
  });
});
