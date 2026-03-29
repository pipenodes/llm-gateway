import type { ReactNode } from 'react';
import { render } from '@testing-library/react';
import { createTheme, ThemeProvider } from '@mui/material/styles';
import CssBaseline from '@mui/material/CssBaseline';

// Wrapper minimalista com ThemeProvider para testes de componentes MUI
const theme = createTheme();

function AllProviders({ children }: { children: ReactNode }) {
  return (
    <ThemeProvider theme={theme}>
      <CssBaseline />
      {children}
    </ThemeProvider>
  );
}

// Re-exporta render com providers já configurados
export function renderWithTheme(ui: ReactNode) {
  return render(ui, { wrapper: AllProviders });
}

// Re-exporta tudo do testing-library para uso direto nos testes
export * from '@testing-library/react';
export { renderWithTheme as render };
