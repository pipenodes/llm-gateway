import { useMemo, useEffect } from 'react';
import type { ReactNode } from 'react';

import { createTheme, StyledEngineProvider, ThemeProvider, useColorScheme } from '@mui/material/styles';
import CssBaseline from '@mui/material/CssBaseline';

import useConfig from 'hooks/useConfig';
import CustomShadows from './custom-shadows';
import componentsOverride from './overrides';
import { buildPalette } from './palette';
import Typography from './typography';

// ── Cross-tab & OS sync (inside ThemeProvider so useColorScheme is available) ──
function ThemeSyncProvider({ children }: { children: ReactNode }) {
  const { setMode } = useColorScheme();

  useEffect(() => {
    // Cross-tab sync — RF-00-UX-DEFAULT-DARKMODE
    const handleStorage = (e: StorageEvent) => {
      if (e.key === 'theme' && e.newValue) {
        setMode(e.newValue as 'light' | 'dark' | 'system');
      }
    };
    window.addEventListener('storage', handleStorage);

    // Real-time OS preference — only when mode is "system"
    const mq = window.matchMedia('(prefers-color-scheme: dark)');
    const handleMq = () => {
      const stored = localStorage.getItem('theme');
      if (stored === 'system' || !stored) {
        // ThemeProvider with defaultMode="system" already handles this;
        // force re-render by toggling to system (no-op if already system)
        setMode('system');
      }
    };
    mq.addEventListener('change', handleMq);

    return () => {
      window.removeEventListener('storage', handleStorage);
      mq.removeEventListener('change', handleMq);
    };
  }, [setMode]);

  return <>{children}</>;
}

export default function ThemeCustomization({ children }: { children: ReactNode }) {
  const { state } = useConfig();

  const themeTypography = useMemo(() => Typography(state.fontFamily), [state.fontFamily]);
  const palette = useMemo(() => buildPalette(), []);

  const themeOptions = useMemo(
    () => ({
      // ── Breakpoints — spec: sm:600 md:900 lg:1200 xl:1536 ────────────────
      breakpoints: {
        values: { xs: 0, sm: 600, md: 900, lg: 1200, xl: 1536 }
      },

      direction: 'ltr' as const,

      mixins: {
        toolbar: { minHeight: 60, paddingTop: 8, paddingBottom: 8 }
      },

      // ── Shape — spec: borderRadius 8px ───────────────────────────────────
      shape: { borderRadius: 8 },

      typography: themeTypography,

      // ── Motion — RF-00-UX-DEFAULT-MOTION ─────────────────────────────────
      transitions: {
        duration: {
          shortest: 150,
          shorter: 200,
          short: 250,
          standard: 300,
          complex: 375,
          // spec: RF-00-UX-DEFAULT-MOTION — valores exatos do spec
          enteringScreen: 225,
          leavingScreen: 195
        },
        easing: {
          // decelerate: element enters screen
          easeOut: 'cubic-bezier(0, 0, 0.2, 1)',
          // accelerate: element exits screen
          easeIn: 'cubic-bezier(0.4, 0, 1, 1)',
          // standard: general movement
          easeInOut: 'cubic-bezier(0.4, 0, 0.2, 1)',
          // fast toggle/collapse
          sharp: 'cubic-bezier(0.4, 0, 0.6, 1)'
        }
      },

      // ── Color schemes (light + dark) ─────────────────────────────────────
      colorSchemes: {
        light: {
          palette: palette.light,
          customShadows: CustomShadows(palette.light as never)
        },
        dark: {
          palette: palette.dark,
          customShadows: CustomShadows(palette.dark as never)
        }
      },

      cssVariables: {
        cssVarPrefix: '',
        colorSchemeSelector: 'data-color-scheme'
      }
    }),
    [themeTypography, palette]
  );

  const themes = createTheme(themeOptions);
  themes.components = componentsOverride(themes);

  return (
    <StyledEngineProvider injectFirst>
      <ThemeProvider
        theme={themes}
        // spec: localStorage key "theme", default: system preference
        modeStorageKey="theme"
        defaultMode="system"
        disableTransitionOnChange
      >
        <CssBaseline enableColorScheme />
        <ThemeSyncProvider>{children}</ThemeSyncProvider>
      </ThemeProvider>
    </StyledEngineProvider>
  );
}
