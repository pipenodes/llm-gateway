#!/usr/bin/env node
/**
 * Validate minimum Product Skeleton adherence.
 *
 * Usage:
 *   node scripts/validate-skeleton.js
 *   node scripts/validate-skeleton.js reports/2026-02-18.report.json
 */

const fs = require("fs");
const path = require("path");

const root = process.cwd();
const reportPath = resolveReportPath(process.argv[2]);
const report = readJsonIfExists(reportPath);
const policy = loadPolicy();

if (!report) {
  console.error(`❌ Report nao encontrado/invalido: ${path.relative(root, reportPath)}`);
  console.error("   Execute antes: node scripts/generate-report.js");
  process.exit(1);
}

const requiredDirs = [
  "src/features/identity",
  "src/features/sharing",
  "src/features/upload",
  "src/features/notifications",
  "src/features/preferences",
  "src/features/security",
  "src/features/licensing",
  "src/features/billing",
  "src/features/payments",
  "src/features/privacy",
  "src/features/terms",
  "src/templates/email"
];

const missing = requiredDirs.filter(rel => !fs.existsSync(path.join(root, rel)));

if (policy.template === "react-vite-mui") {
  const muiThemeExists = fs.existsSync(path.join(root, "src/themes")) || fs.existsSync(path.join(root, "src/theme"));
  if (!muiThemeExists) {
    missing.push("src/themes (ou src/theme)");
  }
}

if (missing.length > 0) {
  console.error("❌ Skeleton gate failed.");
  console.error("   Estruturas obrigatorias ausentes:");
  for (const item of missing) {
    console.error(`   - ${item}`);
  }
  process.exit(1);
}

console.log("✅ Skeleton gate passed.");
console.log(`   Report: ${path.relative(root, reportPath)}`);
console.log(`   Template: ${policy.template || "desconhecido"} (${policy.version || "desconhecida"})`);

function resolveReportPath(cliArg) {
  if (cliArg) {
    return path.resolve(root, cliArg);
  }

  const reportsDir = path.join(root, "reports");
  if (!fs.existsSync(reportsDir)) {
    return path.join(reportsDir, `${new Date().toISOString().slice(0, 10)}.report.json`);
  }

  const reportFiles = fs
    .readdirSync(reportsDir)
    .filter(file => /^\d{4}-\d{2}-\d{2}\.report\.json$/.test(file))
    .sort();

  if (reportFiles.length === 0) {
    return path.join(reportsDir, `${new Date().toISOString().slice(0, 10)}.report.json`);
  }

  return path.join(reportsDir, reportFiles[reportFiles.length - 1]);
}

function readJsonIfExists(filePath) {
  if (!fs.existsSync(filePath)) return null;
  try {
    return JSON.parse(fs.readFileSync(filePath, "utf8"));
  } catch (error) {
    return null;
  }
}

function loadPolicy() {
  const localPath = path.join(root, "policy.json");
  const fallbackPath = path.join(root, ".cursor", "policies", "policy.json");

  if (fs.existsSync(localPath)) {
    return JSON.parse(fs.readFileSync(localPath, "utf8"));
  }
  if (fs.existsSync(fallbackPath)) {
    return JSON.parse(fs.readFileSync(fallbackPath, "utf8"));
  }

  return { template: "unknown", version: "unknown" };
}
