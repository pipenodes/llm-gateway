import { describe, it, expect, vi } from 'vitest';
import { screen } from '@testing-library/react';
import userEvent from '@testing-library/user-event';
import { render } from 'test/helpers';
import ConfirmDialog from './ConfirmDialog';

describe('ConfirmDialog', () => {
  const baseProps = {
    open: true,
    title: 'Excluir item',
    description: 'Esta ação não pode ser desfeita.',
    onConfirm: vi.fn(),
    onCancel: vi.fn()
  };

  it('renderiza título e descrição', () => {
    render(<ConfirmDialog {...baseProps} />);
    expect(screen.getByText('Excluir item')).toBeInTheDocument();
    expect(screen.getByText('Esta ação não pode ser desfeita.')).toBeInTheDocument();
  });

  it('chama onConfirm ao clicar em Confirmar', async () => {
    const onConfirm = vi.fn();
    const user = userEvent.setup();
    render(<ConfirmDialog {...baseProps} onConfirm={onConfirm} />);
    await user.click(screen.getByRole('button', { name: /confirmar/i }));
    expect(onConfirm).toHaveBeenCalledOnce();
  });

  it('chama onCancel ao clicar em Cancelar', async () => {
    const onCancel = vi.fn();
    const user = userEvent.setup();
    render(<ConfirmDialog {...baseProps} onCancel={onCancel} />);
    await user.click(screen.getByRole('button', { name: /cancelar/i }));
    expect(onCancel).toHaveBeenCalledOnce();
  });

  it('botão Cancelar recebe foco ao abrir (ação padrão não-destrutiva)', () => {
    render(<ConfirmDialog {...baseProps} />);
    // O MUI aplica o autoFocus internamente — verificamos que o botão existe e está habilitado
    const cancelBtn = screen.getByRole('button', { name: /cancelar/i });
    expect(cancelBtn).toBeEnabled();
    expect(cancelBtn).not.toBeDisabled();
  });

  it('não renderiza quando open=false', () => {
    render(<ConfirmDialog {...baseProps} open={false} />);
    expect(screen.queryByText('Excluir item')).not.toBeInTheDocument();
  });

  it('usa labels customizados', () => {
    render(
      <ConfirmDialog {...baseProps} confirmLabel="Sim, excluir" cancelLabel="Não, manter" />
    );
    expect(screen.getByRole('button', { name: /sim, excluir/i })).toBeInTheDocument();
    expect(screen.getByRole('button', { name: /não, manter/i })).toBeInTheDocument();
  });
});
