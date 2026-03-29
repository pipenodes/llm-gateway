import { tv } from 'utils/themeVars';
import type { Theme } from '@mui/material/styles';

export default function TableCell(theme: Theme) {
  const p = tv(theme);
  const commonCell = {
    fontSize: '0.75rem',
    textTransform: 'uppercase',
    '&:not(:last-of-type)': {
      backgroundImage: `linear-gradient(${p.divider}, ${p.divider})`,
      backgroundRepeat: 'no-repeat',
      backgroundSize: '1px calc(100% - 30px)',
      backgroundPosition: 'right 16px'
    }
  };

  return {
    MuiTableCell: {
      styleOverrides: {
        root: ({ ownerState }: { ownerState: { align?: string } }) => {
          const base = { fontSize: '0.875rem', padding: 12, borderColor: p.divider };
          if (ownerState.align === 'right') {
            return {
              ...base,
              justifyContent: 'flex-end',
              textAlign: 'right',
              '& > *': { justifyContent: 'flex-end', margin: '0 0 0 auto' },
              '& .MuiOutlinedInput-input': { textAlign: 'right' }
            };
          }
          if (ownerState.align === 'center') {
            return {
              ...base,
              justifyContent: 'center',
              textAlign: 'center',
              '& > *': { justifyContent: 'center', margin: '0 auto' }
            };
          }
          return base;
        },
        sizeSmall: { padding: 8 },
        head: { fontWeight: 700, ...commonCell },
        footer: { ...commonCell }
      }
    }
  };
}
