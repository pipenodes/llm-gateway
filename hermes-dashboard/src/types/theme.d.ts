import '@mui/material/styles';
import type { Theme } from '@mui/material/styles';

declare module '@mui/material/styles' {
  interface Theme {
    customShadows: CustomShadows;
  }

  interface ThemeOptions {
    customShadows?: CustomShadows;
  }

  // Ant Design color scale extras
  interface PaletteColor {
    lighter?: string;
    light: string;
    main: string;
    dark: string;
    darker?: string;
    contrastText: string;
    100?: string;
    200?: string;
    400?: string;
    700?: string;
    900?: string;
  }

  interface PaletteColorChannel {
    lighter?: string;
    darker?: string;
  }

  interface ColorSchemes {
    light?: {
      palette?: import('@mui/material/styles').PaletteOptions;
      customShadows?: CustomShadows;
    };
    dark?: {
      palette?: import('@mui/material/styles').PaletteOptions;
      customShadows?: CustomShadows;
    };
  }
}

interface CustomShadows {
  button: string;
  text: string;
  z1: string;
  primary: string;
  secondary: string;
  error: string;
  warning: string;
  info: string;
  success: string;
  grey: string;
  primaryButton: string;
  secondaryButton: string;
  errorButton: string;
  warningButton: string;
  infoButton: string;
  successButton: string;
  greyButton: string;
}
