#!/usr/bin/env node
/**
 * Lint Gate
 * Runs ESLint where configuration exists.
 */

const { execSync } = require("child_process");
const fs = require("fs");
const path = require("path");

const ESLINT_CONFIG_FILES = [
  "eslint.config.js",
  "eslint.config.mjs",
  "eslint.config.cjs"
];

function hasEslintConfig(dir) {
  return ESLINT_CONFIG_FILES.some(file => fs.existsSync(path.join(dir, file)));
}

function hasPackageJson(dir) {
  return fs.existsSync(path.join(dir, "package.json"));
}

function hasLintScript(dir) {
  const pkgPath = path.join(dir, "package.json");
  if (!fs.existsSync(pkgPath)) return false;
  try {
    const pkg = JSON.parse(fs.readFileSync(pkgPath, "utf8"));
    return Boolean(pkg.scripts && pkg.scripts.lint);
  } catch (e) {
    return false;
  }
}

function getCandidateDirs(rootDir) {
  const candidates = [];
  if (hasEslintConfig(rootDir)) {
    candidates.push(rootDir);
    return candidates;
  }

  const entries = fs.readdirSync(rootDir, { withFileTypes: true });
  for (const entry of entries) {
    if (!entry.isDirectory()) continue;
    if (entry.name.startsWith(".") || entry.name === "node_modules") continue;
    const full = path.join(rootDir, entry.name);
    if (hasPackageJson(full) && hasEslintConfig(full)) {
      candidates.push(full);
    }
  }

  return candidates;
}

const root = process.cwd();
const candidateDirs = getCandidateDirs(root);

if (candidateDirs.length === 0) {
  console.log("⚠️ Nenhuma configuracao ESLint encontrada. Lint gate ignorado.");
  process.exit(0);
}

let failed = false;

for (const dir of candidateDirs) {
  const label = path.relative(root, dir) || ".";
  try {
    console.log(`🔎 Rodando lint em: ${label}`);
    if (hasLintScript(dir)) {
      execSync("npm run lint --silent", { stdio: "inherit", cwd: dir });
    } else {
      execSync("npx eslint .", { stdio: "inherit", cwd: dir });
    }
  } catch (e) {
    failed = true;
    console.error(`❌ Lint falhou em: ${label}`);
  }
}

if (failed) {
  console.error("❌ Lint gate failed.");
  process.exit(1);
}

console.log("✅ Lint gate passed.");
