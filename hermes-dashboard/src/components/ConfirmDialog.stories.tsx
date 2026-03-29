import { useState } from 'react';
import type { Meta, StoryObj } from '@storybook/react';
import Button from '@mui/material/Button';
import ConfirmDialog from './ConfirmDialog';

const meta: Meta<typeof ConfirmDialog> = {
  title: 'Components/ConfirmDialog',
  component: ConfirmDialog,
  tags: ['autodocs'],
  parameters: {
    docs: {
      description: {
        component: 'Diálogo de confirmação para ações destrutivas — RF-00-UX-DEFAULT-FOUNDATION. O foco padrão é no botão Cancelar (ação segura).'
      }
    }
  }
};

export default meta;
type Story = StoryObj<typeof ConfirmDialog>;

// Wrapper interativo para demonstrar abertura/fechamento
function DialogDemo(args: React.ComponentProps<typeof ConfirmDialog>) {
  const [open, setOpen] = useState(false);
  return (
    <>
      <Button variant="contained" color="error" onClick={() => setOpen(true)}>
        Abrir diálogo
      </Button>
      <ConfirmDialog
        {...args}
        open={open}
        onConfirm={() => { alert('Confirmado!'); setOpen(false); }}
        onCancel={() => setOpen(false)}
      />
    </>
  );
}

export const Exclusao: Story = {
  name: 'Excluir registro (destrutivo)',
  render: (args) => <DialogDemo {...args} />,
  args: {
    title: 'Excluir registro',
    description: 'Esta ação não pode ser desfeita. O registro será removido permanentemente.',
    confirmLabel: 'Sim, excluir',
    cancelLabel: 'Cancelar',
    confirmColor: 'error'
  }
};

export const Revogacao: Story = {
  name: 'Revogar acesso (warning)',
  render: (args) => <DialogDemo {...args} />,
  args: {
    title: 'Revogar acesso',
    description: 'O usuário perderá acesso imediatamente. Você poderá conceder novamente depois.',
    confirmLabel: 'Revogar',
    cancelLabel: 'Manter acesso',
    confirmColor: 'warning'
  }
};

export const SemDescricao: Story = {
  name: 'Sem descrição (mínimo)',
  render: (args) => <DialogDemo {...args} />,
  args: {
    title: 'Confirmar operação?',
    confirmLabel: 'Confirmar',
    cancelLabel: 'Cancelar'
  }
};

export const Carregando: Story = {
  name: 'Estado loading',
  args: {
    open: true,
    title: 'Excluindo...',
    description: 'Aguarde enquanto o registro é removido.',
    loading: true,
    onConfirm: () => { },
    onCancel: () => { }
  }
};
