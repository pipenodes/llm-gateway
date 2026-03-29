import { createContext, useCallback, useContext, useRef, useState } from 'react';
import type { ReactNode } from 'react';

import Box from '@mui/material/Box';

// AnnouncerContext — RF-00-UX-DEFAULT-FOUNDATION
// Região aria-live para anunciar feedback de sucesso/erro a leitores de tela.
// Uso: const { announce } = useAnnouncer();
//       announce('Item excluído com sucesso.', 'polite');
//       announce('Erro ao salvar. Tente novamente.', 'assertive');

type Politeness = 'polite' | 'assertive';

interface AnnouncerContextValue {
  /** Anuncia uma mensagem para tecnologias assistivas */
  announce: (message: string, politeness?: Politeness) => void;
}

const AnnouncerContext = createContext<AnnouncerContextValue>({
  announce: () => { }
});

export function useAnnouncer() {
  return useContext(AnnouncerContext);
}

interface AnnouncerProviderProps {
  children: ReactNode;
}

export function AnnouncerProvider({ children }: AnnouncerProviderProps) {
  const [politeMsg, setPoliteMsg] = useState('');
  const [assertiveMsg, setAssertiveMsg] = useState('');
  // Limpa a mensagem após 5s para permitir re-anúncio da mesma mensagem
  const politeTimer = useRef<ReturnType<typeof setTimeout>>();
  const assertiveTimer = useRef<ReturnType<typeof setTimeout>>();

  const announce = useCallback((message: string, politeness: Politeness = 'polite') => {
    if (politeness === 'assertive') {
      setAssertiveMsg('');
      clearTimeout(assertiveTimer.current);
      // Micro-delay para garantir que o DOM perceba a mudança
      setTimeout(() => {
        setAssertiveMsg(message);
        assertiveTimer.current = setTimeout(() => setAssertiveMsg(''), 5000);
      }, 50);
    } else {
      setPoliteMsg('');
      clearTimeout(politeTimer.current);
      setTimeout(() => {
        setPoliteMsg(message);
        politeTimer.current = setTimeout(() => setPoliteMsg(''), 5000);
      }, 50);
    }
  }, []);

  return (
    <AnnouncerContext.Provider value={{ announce }}>
      {children}
      {/* Regiões aria-live: visualmente ocultas mas acessíveis a leitores de tela */}
      <Box
        aria-live="polite"
        aria-atomic="true"
        sx={{
          position: 'absolute',
          width: 1,
          height: 1,
          padding: 0,
          margin: -1,
          overflow: 'hidden',
          clip: 'rect(0,0,0,0)',
          whiteSpace: 'nowrap',
          border: 0
        }}
      >
        {politeMsg}
      </Box>
      <Box
        aria-live="assertive"
        aria-atomic="true"
        sx={{
          position: 'absolute',
          width: 1,
          height: 1,
          padding: 0,
          margin: -1,
          overflow: 'hidden',
          clip: 'rect(0,0,0,0)',
          whiteSpace: 'nowrap',
          border: 0
        }}
      >
        {assertiveMsg}
      </Box>
    </AnnouncerContext.Provider>
  );
}
