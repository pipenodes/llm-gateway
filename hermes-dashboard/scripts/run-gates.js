#!/usr/bin/env node
/**
 * Run all quality gates in order with fail-fast checks.
 */
const { execSync } = require("child_process");
const fs = require("fs");
const path = require("path");

const root = process.cwd();
const scriptDir = __dirname;
const projectScriptsDir = path.join(root, "scripts");
const runtimeScriptsDir = fs.existsSync(projectScriptsDir) ? projectScriptsDir : scriptDir;
const reportPath = resolveReportPath(process.argv[2]);
const schemaPath = path.join(root, ".cursor", "reports", "report.schema.json");

function resolveReportPath(cliArg) {
  if (cliArg) {
    return cliArg;
  }

  const reportsDir = path.join(root, "reports");
  if (!fs.existsSync(reportsDir)) {
    return path.join("reports", `${new Date().toISOString().slice(0, 10)}.report.json`);
  }

  const reportFiles = fs
    .readdirSync(reportsDir)
    .filter(file => /^\d{4}-\d{2}-\d{2}\.report\.json$/.test(file))
    .sort();

  if (reportFiles.length > 0) {
    return path.join("reports", reportFiles[reportFiles.length - 1]);
  }

  return path.join("reports", `${new Date().toISOString().slice(0, 10)}.report.json`);
}

function loadJson(filePath) {
  return JSON.parse(fs.readFileSync(filePath, "utf8"));
}

function validateReport(report, schema) {
  const required = schema.required || [];
  const missing = required.filter(key => !(key in report));
  if (missing.length > 0) {
    return {
      ok: false,
      reason: `Campos obrigatorios ausentes: ${missing.join(", ")}`
    };
  }

  if (!/^\d{4}-\d{2}-\d{2}$/.test(String(report.date || ""))) {
    return { ok: false, reason: "Campo 'date' deve estar no formato YYYY-MM-DD." };
  }

  if (!Array.isArray(report.newDependencies)) {
    return { ok: false, reason: "Campo 'newDependencies' deve ser array." };
  }

  const boolFields = [
    "usesTailwindV4",
    "usesMUI",
    "usesCustomCSS",
    "usesMUIProComponents",
    "dependenciesJustified"
  ];
  for (const field of boolFields) {
    if (typeof report[field] !== "boolean") {
      return { ok: false, reason: `Campo '${field}' deve ser boolean.` };
    }
  }

  if (typeof report.notes !== "string") {
    return { ok: false, reason: "Campo 'notes' deve ser string." };
  }

  return { ok: true };
}

try {
  if (!fs.existsSync(path.join(root, "policy.json")) && !fs.existsSync(path.join(root, ".cursor", "policies", "policy.json"))) {
    console.error("❌ policy.json nao encontrado.");
    console.error("   Esperado em ./policy.json (preferencial) ou ./.cursor/policies/policy.json");
    process.exit(1);
  }

  if (!fs.existsSync(path.join(root, reportPath))) {
    console.log(`ℹ️ Report nao encontrado (${reportPath}). Gerando automaticamente...`);
    execSync(`node "${path.join(runtimeScriptsDir, "generate-report.js")}" --report "${reportPath}"`, { stdio: "inherit" });
  }

  if (!fs.existsSync(schemaPath)) {
    console.error(`❌ Schema de report nao encontrado: ${path.relative(root, schemaPath)}`);
    process.exit(1);
  }

  const report = loadJson(path.join(root, reportPath));
  const schema = loadJson(schemaPath);
  const validation = validateReport(report, schema);
  if (!validation.ok) {
    console.error(`❌ Report invalido (${reportPath}): ${validation.reason}`);
    console.error("   Execute: node scripts/generate-report.js");
    process.exit(1);
  }

  execSync(`node "${path.join(runtimeScriptsDir, "policy-check.js")}" "${reportPath}"`, { stdio: "inherit" });
  execSync(`node "${path.join(runtimeScriptsDir, "lint-gate.js")}"`, { stdio: "inherit" });
  execSync(`node "${path.join(runtimeScriptsDir, "test-gate.js")}"`, { stdio: "inherit" });
  if (fs.existsSync(path.join(runtimeScriptsDir, "validate-skeleton.js"))) {
    execSync(`node "${path.join(runtimeScriptsDir, "validate-skeleton.js")}" "${reportPath}"`, { stdio: "inherit" });
  }
  if (fs.existsSync(path.join(runtimeScriptsDir, "metrics-collector.js"))) {
    execSync(`node "${path.join(runtimeScriptsDir, "metrics-collector.js")}"`, { stdio: "inherit" });
  }
  console.log("✅ All gates passed.");
} catch (e) {
  console.error("❌ Gate sequence failed. Corrija os erros acima e execute novamente.");
  process.exit(1);
}
