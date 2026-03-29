import '@testing-library/jest-dom';
import { server } from 'mocks/server';

// MSW — intercepta chamadas HTTP em testes
beforeAll(() => server.listen({ onUnhandledRequest: 'warn' }));
afterEach(() => server.resetHandlers());
afterAll(() => server.close());

// Mock do i18next para testes — retorna a chave como string traduzida
// Evita necessidade de carregar locale completo em cada teste
vi.mock('react-i18next', () => ({
  useTranslation: () => ({
    t: (key: string, opts?: Record<string, unknown>) => {
      if (!opts) return key;
      // Interpolação simples: substitui {{variavel}} pelo valor de opts.variavel
      return Object.entries(opts).reduce(
        (acc, [k, v]) => acc.replace(new RegExp(`\\{\\{${k}\\}\\}`, 'g'), String(v)),
        key
      );
    },
    i18n: { language: 'pt-BR', changeLanguage: vi.fn(), exists: vi.fn(() => false) }
  }),
  initReactI18next: { type: '3rdParty', init: vi.fn() }
}));

// Mock do react-router-dom para evitar BrowserRouter nos testes de componente
vi.mock('react-router-dom', async (importOriginal) => {
  const actual = await importOriginal<typeof import('react-router-dom')>();
  return {
    ...actual,
    useNavigate: () => vi.fn(),
    useLocation: () => ({ pathname: '/', search: '', hash: '' }),
    useMatches: () => []
  };
});
