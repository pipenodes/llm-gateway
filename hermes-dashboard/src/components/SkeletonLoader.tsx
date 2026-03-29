import Skeleton from '@mui/material/Skeleton';
import Stack from '@mui/material/Stack';
import Box from '@mui/material/Box';
import type { SxProps, Theme } from '@mui/material/styles';

// SkeletonLoader — RF-00-UX-DEFAULT-MOTION
// Componentes de skeleton com shimmer (pulse 1500ms — padrão MUI).
// Exporta variantes para os padrões de loading mais comuns.

// ── Lista de itens ────────────────────────────────────────────────────────────
interface SkeletonListProps {
  rows?: number;
  /** Mostrar avatar à esquerda */
  avatar?: boolean;
  sx?: SxProps<Theme>;
}

export function SkeletonList({ rows = 5, avatar = false, sx }: SkeletonListProps) {
  return (
    <Stack spacing={1.5} sx={sx}>
      {Array.from({ length: rows }).map((_, i) => (
        <Stack key={i} direction="row" spacing={1.5} alignItems="center">
          {avatar && (
            <Skeleton variant="circular" width={40} height={40} animation="wave" />
          )}
          <Stack spacing={0.5} flex={1}>
            <Skeleton variant="text" width="60%" height={20} animation="wave" />
            <Skeleton variant="text" width="90%" height={16} animation="wave" />
          </Stack>
        </Stack>
      ))}
    </Stack>
  );
}

// ── Cards em grid ─────────────────────────────────────────────────────────────
interface SkeletonCardGridProps {
  cards?: number;
  sx?: SxProps<Theme>;
}

export function SkeletonCardGrid({ cards = 3, sx }: SkeletonCardGridProps) {
  return (
    <Box
      sx={{
        display: 'grid',
        gridTemplateColumns: 'repeat(auto-fill, minmax(220px, 1fr))',
        gap: 2,
        ...sx
      }}
    >
      {Array.from({ length: cards }).map((_, i) => (
        <Stack key={i} spacing={1}>
          <Skeleton variant="rectangular" height={140} animation="wave" sx={{ borderRadius: 1 }} />
          <Skeleton variant="text" width="70%" animation="wave" />
          <Skeleton variant="text" width="50%" height={14} animation="wave" />
        </Stack>
      ))}
    </Box>
  );
}

// ── Tabela ────────────────────────────────────────────────────────────────────
interface SkeletonTableProps {
  rows?: number;
  cols?: number;
  sx?: SxProps<Theme>;
}

export function SkeletonTable({ rows = 5, cols = 4, sx }: SkeletonTableProps) {
  return (
    <Stack spacing={0} sx={sx}>
      {/* Header */}
      <Stack direction="row" spacing={2} sx={{ py: 1.5, borderBottom: '1px solid', borderColor: 'divider' }}>
        {Array.from({ length: cols }).map((_, i) => (
          <Skeleton key={i} variant="text" flex={i === 0 ? 2 : 1} height={18} animation="wave" />
        ))}
      </Stack>
      {/* Rows */}
      {Array.from({ length: rows }).map((_, r) => (
        <Stack
          key={r}
          direction="row"
          spacing={2}
          sx={{ py: 1.5, borderBottom: '1px solid', borderColor: 'divider' }}
        >
          {Array.from({ length: cols }).map((_, c) => (
            <Skeleton key={c} variant="text" flex={c === 0 ? 2 : 1} height={18} animation="wave" />
          ))}
        </Stack>
      ))}
    </Stack>
  );
}

// ── Facade default export ─────────────────────────────────────────────────────
interface SkeletonLoaderProps {
  variant?: 'list' | 'card' | 'table' | 'detail';
  rows?: number;
  count?: number;
  cols?: number;
  avatar?: boolean;
  sx?: SxProps<Theme>;
}

export default function SkeletonLoader({ variant = 'list', rows, count, cols, avatar, sx }: SkeletonLoaderProps) {
  switch (variant) {
    case 'card':
      return <SkeletonCardGrid cards={count ?? 3} sx={sx} />;
    case 'table':
      return <SkeletonTable rows={rows ?? 5} cols={cols ?? 4} sx={sx} />;
    case 'detail':
      return <SkeletonDetail sx={sx} />;
    default:
      return <SkeletonList rows={rows ?? 5} avatar={avatar} sx={sx} />;
  }
}

// ── Detalhe (página de detalhe de registro) ───────────────────────────────────
export function SkeletonDetail({ sx }: { sx?: SxProps<Theme> }) {
  return (
    <Stack spacing={3} sx={sx}>
      <Stack spacing={1}>
        <Skeleton variant="text" width="40%" height={32} animation="wave" />
        <Skeleton variant="text" width="60%" height={20} animation="wave" />
      </Stack>
      <Stack spacing={1.5}>
        {Array.from({ length: 6 }).map((_, i) => (
          <Stack key={i} direction="row" spacing={2} alignItems="center">
            <Skeleton variant="text" width={120} height={18} animation="wave" />
            <Skeleton variant="text" flex={1} height={18} animation="wave" />
          </Stack>
        ))}
      </Stack>
    </Stack>
  );
}
