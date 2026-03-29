import type { ReactNode } from 'react';

import Box from '@mui/material/Box';
import Button from '@mui/material/Button';
import Stack from '@mui/material/Stack';
import Typography from '@mui/material/Typography';
import type { SxProps, Theme } from '@mui/material/styles';

// EmptyState — RF-00-UX-DEFAULT-FOUNDATION
// Componente genérico para estados vazios em listas, módulos e painéis.
// Uso: <EmptyState title="Nenhum item" description="..." action={{ label: 'Criar', onClick: () => {} }} />

interface EmptyStateAction {
  label: string;
  onClick: () => void;
  variant?: 'contained' | 'outlined' | 'text';
}

interface EmptyStateProps {
  /** Ícone ou imagem opcional exibido acima do título */
  icon?: ReactNode;
  /** Título principal — obrigatório */
  title: string;
  /** Descrição complementar */
  description?: string;
  /** Ação principal (botão CTA) */
  action?: EmptyStateAction;
  /** Ação secundária */
  secondaryAction?: EmptyStateAction;
  sx?: SxProps<Theme>;
}

export default function EmptyState({
  icon,
  title,
  description,
  action,
  secondaryAction,
  sx
}: EmptyStateProps) {
  return (
    <Box
      role="status"
      aria-label={title}
      sx={{
        display: 'flex',
        flexDirection: 'column',
        alignItems: 'center',
        justifyContent: 'center',
        py: 8,
        px: 3,
        textAlign: 'center',
        gap: 2,
        ...sx
      }}
    >
      {icon && (
        <Box sx={{ fontSize: '3rem', color: 'text.disabled', lineHeight: 1 }}>
          {icon}
        </Box>
      )}

      <Stack spacing={0.5} alignItems="center">
        <Typography variant="h5" color="text.primary">
          {title}
        </Typography>
        {description && (
          <Typography variant="body2" color="text.secondary" sx={{ maxWidth: 380 }}>
            {description}
          </Typography>
        )}
      </Stack>

      {(action || secondaryAction) && (
        <Stack direction="row" spacing={1.5} sx={{ mt: 1 }}>
          {action && (
            <Button
              variant={action.variant ?? 'contained'}
              onClick={action.onClick}
            >
              {action.label}
            </Button>
          )}
          {secondaryAction && (
            <Button
              variant={secondaryAction.variant ?? 'outlined'}
              onClick={secondaryAction.onClick}
            >
              {secondaryAction.label}
            </Button>
          )}
        </Stack>
      )}
    </Box>
  );
}
