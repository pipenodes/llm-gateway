import type { Theme } from '@mui/material/styles';

export default function getShadow(theme: Theme, shadow: string): string {
  const shadows = (theme.vars as unknown as { customShadows: Record<string, string> }).customShadows;
  return shadows?.[shadow] ?? shadows?.primary ?? '';
}
