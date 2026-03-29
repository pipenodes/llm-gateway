import { useColorScheme } from '@mui/material/styles';
import { useTranslation } from 'react-i18next';
import FormControl from '@mui/material/FormControl';
import FormControlLabel from '@mui/material/FormControlLabel';
import FormLabel from '@mui/material/FormLabel';
import Radio from '@mui/material/Radio';
import RadioGroup from '@mui/material/RadioGroup';
import Box from '@mui/material/Box';

// spec: RF-00-UX-DEFAULT-DARKMODE — ThemeToggle exibe RadioGroup com 3 opções visíveis
// O usuário vê o estado atual e pode trocar sem ciclar (Claro / Escuro / Sistema).
type Mode = 'light' | 'dark' | 'system';

interface ThemeToggleMenuItemProps {
  onClose?: () => void;
}

export default function ThemeToggleMenuItem({ onClose }: ThemeToggleMenuItemProps) {
  const { mode, setMode } = useColorScheme();
  const { t } = useTranslation();

  const handleChange = (_: React.ChangeEvent<HTMLInputElement>, value: string) => {
    setMode(value as Mode);
    onClose?.();
  };

  return (
    <Box sx={{ px: 2, py: 1 }}>
      <FormControl component="fieldset" size="small">
        <FormLabel component="legend" sx={{ fontSize: '0.75rem', mb: 0.5 }}>
          {t('profile.theme')}
        </FormLabel>
        <RadioGroup
          value={mode ?? 'system'}
          onChange={handleChange}
          aria-label={t('profile.theme')}
        >
          <FormControlLabel
            value="light"
            control={<Radio size="small" />}
            label={t('profile.themeLight')}
            sx={{ '& .MuiFormControlLabel-label': { fontSize: '0.875rem' } }}
          />
          <FormControlLabel
            value="dark"
            control={<Radio size="small" />}
            label={t('profile.themeDark')}
            sx={{ '& .MuiFormControlLabel-label': { fontSize: '0.875rem' } }}
          />
          <FormControlLabel
            value="system"
            control={<Radio size="small" />}
            label={t('profile.themeSystem')}
            sx={{ '& .MuiFormControlLabel-label': { fontSize: '0.875rem' } }}
          />
        </RadioGroup>
      </FormControl>
    </Box>
  );
}
