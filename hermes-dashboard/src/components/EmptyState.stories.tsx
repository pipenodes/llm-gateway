import type { Meta, StoryObj } from '@storybook/react';
import InboxOutlined from '@ant-design/icons/InboxOutlined';
import SearchOutlined from '@ant-design/icons/SearchOutlined';
import LockOutlined from '@ant-design/icons/LockOutlined';
import EmptyState from './EmptyState';

const meta: Meta<typeof EmptyState> = {
  title: 'Components/EmptyState',
  component: EmptyState,
  tags: ['autodocs'],
  parameters: {
    docs: {
      description: {
        component: 'Estado vazio genérico — RF-00-UX-DEFAULT-FOUNDATION. Use em listas, painéis e módulos sem dados.'
      }
    }
  }
};

export default meta;
type Story = StoryObj<typeof EmptyState>;

export const SemDados: Story = {
  name: 'Sem dados (mínimo)',
  args: {
    title: 'Nenhum item encontrado'
  }
};

export const ComDescricao: Story = {
  name: 'Com descrição',
  args: {
    icon: <InboxOutlined style={{ fontSize: '3rem' }} />,
    title: 'Nenhum registro',
    description: 'Adicione o primeiro item para começar a usar este módulo.'
  }
};

export const ComAcao: Story = {
  name: 'Com ação CTA',
  args: {
    icon: <InboxOutlined style={{ fontSize: '3rem' }} />,
    title: 'Nenhum item',
    description: 'Crie seu primeiro item para visualizá-lo aqui.',
    action: { label: 'Criar item', onClick: () => alert('Criar!') }
  }
};

export const ComDuasAcoes: Story = {
  name: 'Com 2 ações',
  args: {
    icon: <SearchOutlined style={{ fontSize: '3rem' }} />,
    title: 'Nenhum resultado para sua busca',
    description: 'Tente ajustar os filtros ou criar um novo registro.',
    action: { label: 'Limpar filtros', onClick: () => { } },
    secondaryAction: { label: 'Criar novo', onClick: () => { }, variant: 'outlined' }
  }
};

export const SemPermissao: Story = {
  name: 'Sem permissão (RBAC)',
  args: {
    icon: <LockOutlined style={{ fontSize: '3rem' }} />,
    title: 'Acesso restrito',
    description: 'Você não tem permissão para visualizar este conteúdo. Contate o administrador.'
  }
};
