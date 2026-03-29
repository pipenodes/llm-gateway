import { useEffect } from 'react';
import { useLocation } from 'react-router-dom';

// useFocusOnRouteChange — RF-00-UX-DEFAULT-FOUNDATION (a11y WCAG 2.1 — SC 2.4.3)
// Move o foco para o elemento principal da página (#main-content) a cada troca de rota.
// Isso garante que leitores de tela anunciem a nova página corretamente.
//
// Uso: chamar uma vez no componente de layout raiz (DashboardLayout).
// Requisito: o elemento <main> deve ter id="main-content" e tabIndex={-1}.

export function useFocusOnRouteChange() {
  const { pathname } = useLocation();

  useEffect(() => {
    // Aguarda o próximo frame para garantir que o DOM já foi atualizado
    const frame = requestAnimationFrame(() => {
      const main = document.getElementById('main-content');
      if (main) {
        main.focus({ preventScroll: false });
      }
    });
    return () => cancelAnimationFrame(frame);
  }, [pathname]);
}
