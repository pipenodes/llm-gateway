import { describe, it, expect, vi } from 'vitest';
import { screen, act, waitFor } from '@testing-library/react';
import userEvent from '@testing-library/user-event';
import { render } from 'test/helpers';
import { AnnouncerProvider, useAnnouncer } from './AnnouncerContext';

// Componente auxiliar para testar o hook
function TestAnnouncer() {
  const { announce } = useAnnouncer();
  return (
    <div>
      <button onClick={() => announce('Salvo com sucesso', 'polite')}>
        Anunciar polite
      </button>
      <button onClick={() => announce('Erro crítico', 'assertive')}>
        Anunciar assertive
      </button>
    </div>
  );
}

describe('AnnouncerContext', () => {
  it('renderiza regiões aria-live polite e assertive', () => {
    render(
      <AnnouncerProvider>
        <div />
      </AnnouncerProvider>
    );
    expect(document.querySelector('[aria-live="polite"]')).toBeInTheDocument();
    expect(document.querySelector('[aria-live="assertive"]')).toBeInTheDocument();
  });

  it('anuncia mensagem polite', async () => {
    const user = userEvent.setup();
    render(
      <AnnouncerProvider>
        <TestAnnouncer />
      </AnnouncerProvider>
    );
    await user.click(screen.getByText('Anunciar polite'));
    await waitFor(() => {
      const politeEl = document.querySelector('[aria-live="polite"]');
      expect(politeEl?.textContent).toBe('Salvo com sucesso');
    });
  });

  it('anuncia mensagem assertive', async () => {
    const user = userEvent.setup();
    render(
      <AnnouncerProvider>
        <TestAnnouncer />
      </AnnouncerProvider>
    );
    await user.click(screen.getByText('Anunciar assertive'));
    await waitFor(() => {
      const assertiveEl = document.querySelector('[aria-live="assertive"]');
      expect(assertiveEl?.textContent).toBe('Erro crítico');
    });
  });
});
