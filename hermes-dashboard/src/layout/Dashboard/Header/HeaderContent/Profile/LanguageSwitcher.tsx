import { useTranslation } from 'react-i18next';

import Box from '@mui/material/Box';
import FormControl from '@mui/material/FormControl';
import FormControlLabel from '@mui/material/FormControlLabel';
import FormLabel from '@mui/material/FormLabel';
import Radio from '@mui/material/Radio';
import RadioGroup from '@mui/material/RadioGroup';

// Idiomas disponíveis no template — adicionar entradas em src/locales/ para novos idiomas
const LANGUAGES = [
  { code: 'pt-BR', label: 'Português (Brasil)' },
  { code: 'en-US', label: 'English (US)' }
] as const;

interface LanguageSwitcherProps {
  onClose?: () => void;
}

export default function LanguageSwitcher({ onClose }: LanguageSwitcherProps) {
  const { i18n, t } = useTranslation();

  const handleChange = (_: React.ChangeEvent<HTMLInputElement>, value: string) => {
    i18n.changeLanguage(value);
    onClose?.();
  };

  return (
    <Box sx={{ px: 2, py: 1 }}>
      <FormControl component="fieldset" size="small">
        <FormLabel component="legend" sx={{ fontSize: '0.75rem', mb: 0.5 }}>
          {t('profile.language')}
        </FormLabel>
        <RadioGroup
          value={i18n.language}
          onChange={handleChange}
          aria-label={t('profile.language')}
        >
          {LANGUAGES.map((lang) => (
            <FormControlLabel
              key={lang.code}
              value={lang.code}
              control={<Radio size="small" />}
              label={lang.label}
              sx={{ '& .MuiFormControlLabel-label': { fontSize: '0.875rem' } }}
            />
          ))}
        </RadioGroup>
      </FormControl>
    </Box>
  );
}
