import type { Theme, Palette } from '@mui/material/styles';

// Em MUI v7 com colorSchemes, theme.vars.palette contém referências CSS var()
// que respondem automaticamente à troca de tema (light/dark).
// Fallback: theme.palette para contextos sem CSS Variables.
export function tv(theme: Theme): Palette {
  const vars = (theme as unknown as { vars?: { palette?: Palette } }).vars;
  return (vars?.palette ?? theme.palette) as unknown as Palette;
}
