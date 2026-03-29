import type { ReactNode } from 'react';
import { createContext, useCallback, useContext, useMemo, useState } from 'react';
import { EMPRESAS, getEmpresaById } from 'mocks/empresas';

const STORAGE_KEY = 'empresa-ativa';

interface EmpresaAtivaContextValue {
  empresaId: string | null;
  setEmpresaId: (id: string | null) => void;
  empresas: typeof EMPRESAS;
  empresaAtiva: ReturnType<typeof getEmpresaById>;
}

const EmpresaAtivaContext = createContext<EmpresaAtivaContextValue | null>(null);

export function EmpresaAtivaProvider({ children }: { children: ReactNode }) {
  const [empresaId, setEmpresaIdState] = useState<string | null>(() => {
    if (typeof window === 'undefined') return EMPRESAS[0]?.empresaId ?? null;
    const stored = localStorage.getItem(STORAGE_KEY);
    if (stored && EMPRESAS.some((e) => e.empresaId === stored)) return stored;
    return EMPRESAS[0]?.empresaId ?? null;
  });

  const setEmpresaId = useCallback((id: string | null) => {
    setEmpresaIdState(id);
    if (typeof window !== 'undefined') {
      if (id) localStorage.setItem(STORAGE_KEY, id);
      else localStorage.removeItem(STORAGE_KEY);
    }
  }, []);

  const value = useMemo<EmpresaAtivaContextValue>(
    () => ({
      empresaId,
      setEmpresaId,
      empresas: EMPRESAS,
      empresaAtiva: empresaId ? getEmpresaById(empresaId) : undefined
    }),
    [empresaId, setEmpresaId]
  );

  return (
    <EmpresaAtivaContext.Provider value={value}>{children}</EmpresaAtivaContext.Provider>
  );
}

export function useEmpresaAtiva(): EmpresaAtivaContextValue {
  const ctx = useContext(EmpresaAtivaContext);
  if (!ctx) throw new Error('useEmpresaAtiva must be used within EmpresaAtivaProvider');
  return ctx;
}
