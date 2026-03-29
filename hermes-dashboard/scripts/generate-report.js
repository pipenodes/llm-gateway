#!/usr/bin/env node
/**
 * Generate or refresh daily quality report.
 *
 * Usage:
 *   node scripts/generate-report.js
 *   node scripts/generate-report.js --report reports/2026-02-18.report.json
 */

const fs = require("fs");
const path = require("path");

const args = process.argv.slice(2);
const reportArgIndex = args.indexOf("--report");
const reportArg = reportArgIndex >= 0 ? args[reportArgIndex + 1] : null;

const root = process.cwd();
const policy = loadPolicy(root);
const today = new Date().toISOString().slice(0, 10);
const reportPath = reportArg || path.join("reports", `${today}.report.json`);
const absReportPath = path.join(root, reportPath);
const existing = readJsonIfExists(absReportPath) || {};

const packageJson = readJsonIfExists(path.join(root, "package.json")) || {};
const dependencies = {
  ...(packageJson.dependencies || {}),
  ...(packageJson.devDependencies || {})
};

const scan = scanWorkspace(root);
const usesTailwindV4 = Boolean(dependencies.tailwindcss) || scan.tailwindImports > 0 || scan.tailwindDirectives > 0;
const usesMUI = hasMuiDependency(dependencies) || scan.muiImports > 0;
const usesMUIProComponents = scan.muiProImports > 0;
const usesCustomCSS = scan.customCssFiles.length > 0;
const newDependencies = Array.isArray(existing.newDependencies) ? existing.newDependencies : [];
const dependenciesJustified =
  newDependencies.length === 0 ? true : Boolean(existing.dependenciesJustified);

const report = {
  date: today,
  template: policy.template || "unknown",
  version: policy.version || "unknown",
  usesTailwindV4,
  usesMUI,
  usesCustomCSS,
  usesMUIProComponents,
  newDependencies,
  dependenciesJustified,
  notes: typeof existing.notes === "string" ? existing.notes : "",
  generatedAt: new Date().toISOString(),
  generatedBy: "scripts/generate-report.js"
};

ensureDir(path.dirname(absReportPath));
fs.writeFileSync(absReportPath, `${JSON.stringify(report, null, 2)}\n`, "utf8");
console.log(`✅ Report gerado: ${reportPath}`);

function loadPolicy(cwd) {
  const localPath = path.join(cwd, "policy.json");
  const fallbackPath = path.join(cwd, ".cursor", "policies", "policy.json");

  if (fs.existsSync(localPath)) {
    return JSON.parse(fs.readFileSync(localPath, "utf8"));
  }
  if (fs.existsSync(fallbackPath)) {
    return JSON.parse(fs.readFileSync(fallbackPath, "utf8"));
  }

  throw new Error(
    "Nenhum policy.json encontrado. Esperado em ./policy.json ou ./.cursor/policies/policy.json"
  );
}

function ensureDir(dirPath) {
  fs.mkdirSync(dirPath, { recursive: true });
}

function readJsonIfExists(filePath) {
  if (!fs.existsSync(filePath)) return null;
  try {
    return JSON.parse(fs.readFileSync(filePath, "utf8"));
  } catch (error) {
    return null;
  }
}

function hasMuiDependency(deps) {
  return Object.keys(deps).some(name => name === "@mui/material" || name.startsWith("@mui/"));
}

function scanWorkspace(cwd) {
  const srcPath = path.join(cwd, "src");
  const results = {
    tailwindImports: 0,
    tailwindDirectives: 0,
    muiImports: 0,
    muiProImports: 0,
    customCssFiles: []
  };

  if (!fs.existsSync(srcPath)) {
    return results;
  }

  walk(srcPath, filePath => {
    const rel = path.relative(cwd, filePath).replace(/\\/g, "/");
    const ext = path.extname(filePath).toLowerCase();

    if ([".js", ".jsx", ".ts", ".tsx", ".css", ".scss"].includes(ext)) {
      const content = fs.readFileSync(filePath, "utf8");
      if (content.includes("tailwindcss")) results.tailwindImports += 1;
      if (/@tailwind\s+(base|components|utilities)/.test(content)) results.tailwindDirectives += 1;
      if (/from\s+['"]@mui\//.test(content) || /require\(['"]@mui\//.test(content)) results.muiImports += 1;
      if (
        /from\s+['"]@mui\/x-(data-grid-pro|data-grid-premium|date-pickers-pro|charts-pro|tree-view-pro)/.test(content) ||
        /require\(['"]@mui\/x-(data-grid-pro|data-grid-premium|date-pickers-pro|charts-pro|tree-view-pro)/.test(content)
      ) {
        results.muiProImports += 1;
      }
    }

    if ([".css", ".scss"].includes(ext)) {
      const isIgnoredCss =
        rel.endsWith("tailwind.css") || rel.endsWith("globals.css") || rel.includes("/themes/");
      if (!isIgnoredCss) {
        results.customCssFiles.push(rel);
      }
    }
  });

  return results;
}

function walk(startDir, onFile) {
  const entries = fs.readdirSync(startDir, { withFileTypes: true });
  for (const entry of entries) {
    if (entry.name === "node_modules" || entry.name.startsWith(".")) continue;
    const fullPath = path.join(startDir, entry.name);
    if (entry.isDirectory()) {
      walk(fullPath, onFile);
    } else if (entry.isFile()) {
      onFile(fullPath);
    }
  }
}
