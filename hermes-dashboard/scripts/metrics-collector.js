#!/usr/bin/env node
/**
 * Aggregate compliance metrics from daily reports.
 *
 * Usage:
 *   node scripts/metrics-collector.js
 *   node scripts/metrics-collector.js --out reports/metrics-dashboard.md --json reports/metrics-summary.json
 */

const fs = require("fs");
const path = require("path");

const args = process.argv.slice(2);
const outPath = getArgValue(args, "--out") || path.join("reports", "metrics-dashboard.md");
const jsonPath = getArgValue(args, "--json") || null;

const root = process.cwd();
const reportsDir = path.join(root, "reports");
const reports = loadReports(reportsDir);

if (reports.length === 0) {
  console.log("⚠️ Nenhum report encontrado para coleta de metricas.");
  process.exit(0);
}

const summary = buildSummary(reports);
const markdown = renderMarkdown(summary);

ensureDir(path.dirname(path.join(root, outPath)));
fs.writeFileSync(path.join(root, outPath), `${markdown}\n`, "utf8");
console.log(`✅ Dashboard de metricas gerado: ${outPath}`);

if (jsonPath) {
  ensureDir(path.dirname(path.join(root, jsonPath)));
  fs.writeFileSync(path.join(root, jsonPath), `${JSON.stringify(summary, null, 2)}\n`, "utf8");
  console.log(`✅ Sumario JSON gerado: ${jsonPath}`);
}

function getArgValue(argv, key) {
  const i = argv.indexOf(key);
  if (i === -1) return null;
  return argv[i + 1] || null;
}

function ensureDir(dirPath) {
  fs.mkdirSync(dirPath, { recursive: true });
}

function loadReports(dir) {
  if (!fs.existsSync(dir)) return [];
  return fs
    .readdirSync(dir)
    .filter(file => /^\d{4}-\d{2}-\d{2}\.report\.json$/.test(file))
    .sort()
    .map(file => {
      const fullPath = path.join(dir, file);
      try {
        const parsed = JSON.parse(fs.readFileSync(fullPath, "utf8"));
        return { file, ...parsed };
      } catch (error) {
        return null;
      }
    })
    .filter(Boolean);
}

function buildSummary(reports) {
  const total = reports.length;
  const latest = reports[reports.length - 1];
  const templateCount = {};
  const versionCount = {};

  let customCssViolations = 0;
  let tailwindInMuiViolations = 0;
  let missingMUIViolations = 0;
  let unapprovedDeps = 0;
  let muiProUsage = 0;

  for (const report of reports) {
    templateCount[report.template || "unknown"] = (templateCount[report.template || "unknown"] || 0) + 1;
    versionCount[report.version || "unknown"] = (versionCount[report.version || "unknown"] || 0) + 1;

    if (report.usesCustomCSS) customCssViolations += 1;
    if (report.template === "react-vite-mui" && report.usesTailwindV4) tailwindInMuiViolations += 1;
    if (report.template === "react-vite-mui" && !report.usesMUI) missingMUIViolations += 1;
    if (Array.isArray(report.newDependencies) && report.newDependencies.length > 0 && !report.dependenciesJustified) {
      unapprovedDeps += 1;
    }
    if (report.usesMUIProComponents) muiProUsage += 1;
  }

  return {
    generatedAt: new Date().toISOString(),
    totalReports: total,
    period: {
      start: reports[0].date || "unknown",
      end: latest.date || "unknown"
    },
    latestReport: latest.file,
    templates: templateCount,
    versions: versionCount,
    indicators: {
      customCssViolations,
      tailwindInMuiViolations,
      missingMUIViolations,
      unapprovedDeps,
      muiProUsage
    }
  };
}

function renderMarkdown(summary) {
  return [
    "# Compliance Metrics Dashboard",
    "",
    `- Gerado em: ${summary.generatedAt}`,
    `- Periodo: ${summary.period.start} -> ${summary.period.end}`,
    `- Reports analisados: ${summary.totalReports}`,
    `- Ultimo report: ${summary.latestReport}`,
    "",
    "## Distribuicao por Template",
    "",
    ...toBulletList(summary.templates),
    "",
    "## Distribuicao por Versao",
    "",
    ...toBulletList(summary.versions),
    "",
    "## Indicadores de Conformidade",
    "",
    `- Violacoes de CSS customizado: ${summary.indicators.customCssViolations}`,
    `- Tailwind em template MUI: ${summary.indicators.tailwindInMuiViolations}`,
    `- Ausencia de MUI em template MUI: ${summary.indicators.missingMUIViolations}`,
    `- Dependencias sem justificativa: ${summary.indicators.unapprovedDeps}`,
    `- Uso de componentes MUI Pro: ${summary.indicators.muiProUsage}`
  ].join("\n");
}

function toBulletList(record) {
  const keys = Object.keys(record);
  if (keys.length === 0) return ["- N/A"];
  return keys.map(key => `- ${key}: ${record[key]}`);
}
