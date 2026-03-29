/**
 * Mocks de empresas — lista mínima para seletor de empresa ativa e gráfico Dashboard.
 * Em produtos reais, estender com mais campos (cnpj, endereço, etc.).
 */

export interface Empresa {
  empresaId: string;
  nome: string;
}

export const EMPRESAS: Empresa[] = [
  { empresaId: 'emp-001', nome: 'Indústria Alpha Ltda' },
  { empresaId: 'emp-002', nome: 'Comercial Beta S.A.' },
  { empresaId: 'emp-003', nome: 'Serviços Gamma Ltda' },
  { empresaId: 'emp-004', nome: 'Logística Delta Transportes' },
  { empresaId: 'emp-005', nome: 'Tech Epsilon Soluções' }
];

export function getEmpresaById(empresaId: string): Empresa | undefined {
  return EMPRESAS.find((e) => e.empresaId === empresaId);
}
