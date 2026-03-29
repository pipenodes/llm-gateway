import { describe, it, expect, vi } from 'vitest';
import { screen } from '@testing-library/react';
import userEvent from '@testing-library/user-event';
import { render } from 'test/helpers';
import EmptyState from './EmptyState';

describe('EmptyState', () => {
  it('renderiza título obrigatório', () => {
    render(<EmptyState title="Nenhum item encontrado" />);
    expect(screen.getByText('Nenhum item encontrado')).toBeInTheDocument();
  });

  it('renderiza descrição quando fornecida', () => {
    render(<EmptyState title="Vazio" description="Adicione itens para começar." />);
    expect(screen.getByText('Adicione itens para começar.')).toBeInTheDocument();
  });

  it('renderiza botão de ação e chama callback ao clicar', async () => {
    const onAction = vi.fn();
    const user = userEvent.setup();
    render(<EmptyState title="Vazio" action={{ label: 'Criar item', onClick: onAction }} />);
    await user.click(screen.getByRole('button', { name: 'Criar item' }));
    expect(onAction).toHaveBeenCalledOnce();
  });

  it('não renderiza botão quando action não é fornecida', () => {
    render(<EmptyState title="Vazio" />);
    expect(screen.queryByRole('button')).not.toBeInTheDocument();
  });

  it('tem role=status para acessibilidade', () => {
    render(<EmptyState title="Vazio" />);
    expect(screen.getByRole('status')).toBeInTheDocument();
  });
});
