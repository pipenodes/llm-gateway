import i18n from 'i18next';
import { initReactI18next } from 'react-i18next';

import ptBR from 'locales/pt-BR';
import enUS from 'locales/en-US';

// i18n — RF-00-UX-DEFAULT-FOUNDATION
// Configuração central de internacionalização.
// Para adicionar idioma: criar src/locales/<lang>.ts e registrar em `resources`.
//
// Idioma ativo é lido do localStorage (chave: 'language').
// O produto pode sobrescrever via i18n.changeLanguage('en-US').

const savedLanguage = localStorage.getItem('language') ?? 'pt-BR';

i18n
  .use(initReactI18next)
  .init({
    resources: {
      'pt-BR': ptBR,
      'en-US': enUS,
    },
    lng: savedLanguage,
    fallbackLng: 'pt-BR',
    interpolation: {
      // React já escapa strings — não é necessário escaping adicional
      escapeValue: false,
    },
    // Persistir troca de idioma no localStorage
    react: {
      useSuspense: false,
    },
  });

// Persist language changes automatically
i18n.on('languageChanged', (lng) => {
  localStorage.setItem('language', lng);
  document.documentElement.setAttribute('lang', lng.split('-')[0]);
});

export default i18n;
