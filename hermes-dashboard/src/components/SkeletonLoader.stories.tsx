import type { Meta, StoryObj } from '@storybook/react';
import Box from '@mui/material/Box';
import Typography from '@mui/material/Typography';
import { SkeletonList, SkeletonCardGrid, SkeletonTable, SkeletonDetail } from './SkeletonLoader';

const meta: Meta = {
  title: 'Components/SkeletonLoader',
  tags: ['autodocs'],
  parameters: {
    docs: {
      description: {
        component: 'Skeletons com shimmer (wave animation 1500ms) — RF-00-UX-DEFAULT-MOTION. Use como fallback de loading de conteúdo, não de página.'
      }
    }
  }
};

export default meta;

export const Lista: StoryObj = {
  name: 'SkeletonList — lista simples',
  render: () => (
    <Box sx={{ maxWidth: 480 }}>
      <Typography variant="caption" color="text.secondary" sx={{ mb: 1, display: 'block' }}>
        5 linhas sem avatar
      </Typography>
      <SkeletonList rows={5} />
    </Box>
  )
};

export const ListaComAvatar: StoryObj = {
  name: 'SkeletonList — com avatar',
  render: () => (
    <Box sx={{ maxWidth: 480 }}>
      <SkeletonList rows={4} avatar />
    </Box>
  )
};

export const CardGrid: StoryObj = {
  name: 'SkeletonCardGrid — 3 cards',
  render: () => (
    <Box sx={{ maxWidth: 800 }}>
      <SkeletonCardGrid cards={3} />
    </Box>
  )
};

export const Tabela: StoryObj = {
  name: 'SkeletonTable — 5 linhas × 4 colunas',
  render: () => (
    <Box sx={{ maxWidth: 800 }}>
      <SkeletonTable rows={5} cols={4} />
    </Box>
  )
};

export const Detalhe: StoryObj = {
  name: 'SkeletonDetail — página de detalhe',
  render: () => (
    <Box sx={{ maxWidth: 600 }}>
      <SkeletonDetail />
    </Box>
  )
};
