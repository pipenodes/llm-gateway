#!/usr/bin/env node
/**
 * Test Gate
 * Runs unit/integration tests
 */

const { execSync } = require("child_process");
const fs = require("fs");
const path = require("path");

const pkgPath = path.join(process.cwd(), "package.json");

if (!fs.existsSync(pkgPath)) {
  console.log("⚠️ package.json nao encontrado. Test gate ignorado.");
  process.exit(0);
}

let pkg;
try {
  pkg = JSON.parse(fs.readFileSync(pkgPath, "utf8"));
} catch (e) {
  console.error("❌ package.json invalido. Nao foi possivel ler scripts de teste.");
  process.exit(1);
}

if (!pkg.scripts || !pkg.scripts.test) {
  console.log("⚠️ Script de teste nao encontrado em package.json. Test gate ignorado.");
  process.exit(0);
}

try {
  execSync("npm run test -- --watch=false", { stdio: "inherit" });
  console.log("✅ Test gate passed.");
} catch (e) {
  console.error("❌ Test gate failed. Corrija os testes e execute novamente.");
  process.exit(1);
}
