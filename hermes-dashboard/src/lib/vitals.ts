// vitals.ts — Web Vitals monitoring (v5)
// RF-00-UX-DEFAULT-FOUNDATION: métricas de performance UX
// web-vitals v5 removeu onFID → substituído por onINP (mais preciso)
//
// Métricas Core Web Vitals monitoradas:
//   LCP  — Largest Contentful Paint (loading)
//   INP  — Interaction to Next Paint (interatividade, substitui FID)
//   CLS  — Cumulative Layout Shift (estabilidade visual)
//   FCP  — First Contentful Paint (percepção de velocidade)
//   TTFB — Time to First Byte (backend/rede)
//
// Em produção, substitua o console por envio para analytics/APM.

import type { Metric } from 'web-vitals';

const THRESHOLDS = {
  LCP: { good: 2500, poor: 4000 }, // ms
  INP: { good: 200, poor: 500 }, // ms (substitui FID)
  CLS: { good: 0.1, poor: 0.25 }, // score (sem unidade)
  FCP: { good: 1800, poor: 3000 }, // ms
  TTFB: { good: 800, poor: 1800 }  // ms
};

function getRating(name: string, value: number): 'good' | 'needs-improvement' | 'poor' {
  const t = THRESHOLDS[name as keyof typeof THRESHOLDS];
  if (!t) return 'good';
  if (value <= t.good) return 'good';
  if (value <= t.poor) return 'needs-improvement';
  return 'poor';
}

function reportMetric(metric: Metric) {
  const rating = getRating(metric.name, metric.value);
  const emoji = { good: '✅', 'needs-improvement': '⚠️', poor: '❌' }[rating];
  const unit = metric.name === 'CLS' ? '' : 'ms';

  if (import.meta.env.DEV) {
    console.groupCollapsed(
      `${emoji} [Web Vital] ${metric.name}: ${metric.value.toFixed(1)}${unit} (${rating})`
    );
    console.info('id:', metric.id);
    console.info('delta:', metric.delta);
    console.info('rating:', rating);
    console.groupEnd();
  }

  // TODO (produção): enviar para backend / analytics
  // fetch('/api/metrics', {
  //   method: 'POST',
  //   headers: { 'Content-Type': 'application/json' },
  //   body: JSON.stringify({ name: metric.name, value: metric.value, rating, id: metric.id })
  // });
}

export async function initWebVitals() {
  const { onCLS, onFCP, onLCP, onINP, onTTFB } = await import('web-vitals');
  onCLS(reportMetric);
  onFCP(reportMetric);
  onLCP(reportMetric);
  onINP(reportMetric);
  onTTFB(reportMetric);
}
