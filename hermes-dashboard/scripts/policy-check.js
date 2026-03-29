#!/usr/bin/env node
/**
 * Policy Engine (OPA/Rego Simulation)
 * 
 * Suporta multiplos templates e valida imports proibidos.
 * 
 * Uso: node scripts/policy-check.js [report.json] [--scan-imports] [--verbose]
 * 
 * Opcoes:
 *   --scan-imports   Escaneia arquivos em busca de imports proibidos
 *   --verbose        Mostra detalhes do scan
 */

const fs = require("fs");
const path = require("path");

// Carregar politica local (do projeto) ou fallback global
let policy;
const localPolicyPath = path.join(process.cwd(), "policy.json");
const globalPolicyPath = path.join(__dirname, "..", "policies", "policy.json");

if (fs.existsSync(localPolicyPath)) {
  policy = JSON.parse(fs.readFileSync(localPolicyPath));
} else if (fs.existsSync(globalPolicyPath)) {
  policy = JSON.parse(fs.readFileSync(globalPolicyPath));
} else {
  console.error("❌ Nenhum arquivo de politica encontrado.");
  console.error("   Esperado: ./policy.json (preferencial) ou .cursor/policies/policy.json");
  process.exit(1);
}

const args = process.argv.slice(2);
const scanImports = args.includes("--scan-imports");
const verbose = args.includes("--verbose");
const positionalArgs = args.filter(arg => !arg.startsWith("--"));

// Carregar report
const reportPath = resolveReportPath(positionalArgs[0]);
if (!fs.existsSync(reportPath)) {
  console.error("❌ Arquivo de report nao encontrado:", reportPath);
  console.error("   Execute antes: node scripts/generate-report.js");
  process.exit(1);
}

const report = JSON.parse(fs.readFileSync(reportPath, "utf8"));

let violations = [];
let warnings = [];

// ==============================
// Regras do Template Tailwind
// ==============================
if (policy.template === "react-vite-tailwind") {
  if (policy.rules.tailwind_v4_required && !report.usesTailwindV4) {
    violations.push("Tailwind CSS v4 e obrigatorio.");
  }

  if (policy.rules.custom_css_forbidden && report.usesCustomCSS) {
    violations.push("CSS customizado e proibido.");
  }

  if (policy.rules.inline_styles_forbidden && report.usesInlineStyles) {
    violations.push("Estilos inline sao proibidos.");
  }

  if (policy.rules.mui_forbidden && report.usesMUI) {
    violations.push("MUI e proibido neste template.");
  }
}

// ==============================
// Regras do Template MUI
// ==============================
if (policy.template === "react-vite-mui") {
  if (policy.rules.mui_required && !report.usesMUI) {
    violations.push("MUI e obrigatorio neste template.");
  }

  if (policy.rules.tailwind_forbidden && report.usesTailwindV4) {
    violations.push("Tailwind CSS e proibido neste template.");
  }

  if (policy.rules.mui_pro_components_forbidden && report.usesMUIProComponents) {
    violations.push("Componentes MUI PRO sao proibidos na versao FREE.");
  }
}

// ==============================
// Regras Globais
// ==============================
if (
  policy.rules.dependencies_require_justification &&
  report.newDependencies &&
  report.newDependencies.length > 0 &&
  !report.dependenciesJustified
) {
  violations.push("Novas dependencias requerem justificativa.");
}

// ==============================
// Scan de Imports Proibidos
// ==============================
if (scanImports) {
  console.log("Escaneando imports proibidos...\n");
  
  const srcDir = path.join(process.cwd(), "src");
  const examplesDir = path.join(process.cwd(), "examples");
  
  if (fs.existsSync(srcDir)) {
    // Imports proibidos da politica
    if (policy.forbidden_imports && policy.forbidden_imports.length > 0) {
      if (verbose) {
        console.log("Imports proibidos:");
        policy.forbidden_imports.forEach(imp => console.log(`  - ${imp}`));
        console.log("");
      }
      
      const forbiddenFound = scanForForbiddenImports(srcDir, policy.forbidden_imports);
      
      forbiddenFound.forEach(({ file, importName, line }) => {
        violations.push(`Import proibido '${importName}' em ${file}${line ? `:${line}` : ''}`);
      });
      
      if (forbiddenFound.length === 0 && verbose) {
        console.log("Nenhum import proibido encontrado em src/\n");
      }
    }
    
    // Verificar imports Pro em versao Free
    if (policy.version === "free" && policy.template === "react-vite-mui") {
      const proImports = [
        "@mui/x-data-grid-pro",
        "@mui/x-data-grid-premium",
        "@mui/x-date-pickers-pro",
        "@mui/x-charts-pro",
        "@mui/x-tree-view-pro"
      ];
      
      const proFound = scanForForbiddenImports(srcDir, proImports);
      
      proFound.forEach(({ file, importName, line }) => {
        violations.push(`Componente MUI Pro '${importName}' nao permitido em versao FREE (${file}${line ? `:${line}` : ''})`);
      });
    }
  }
  
  // Verificar se examples/ ainda existe (aviso)
  if (fs.existsSync(examplesDir)) {
    warnings.push("Pasta examples/ ainda existe. Remova antes da entrega: node scripts/remove-examples.js");
  }
}

// ==============================
// Resultado
// ==============================

// Mostrar avisos
if (warnings.length > 0) {
  console.warn("");
  console.warn("⚠️  AVISOS:");
  console.warn("");
  warnings.forEach((w, i) => console.warn(`  ${i + 1}. ${w}`));
  console.warn("");
}

// Mostrar violacoes
if (violations.length > 0) {
  console.error("");
  console.error("❌ VIOLACOES DE POLITICA:");
  console.error("");
  violations.forEach((v, i) => console.error(`  ${i + 1}. ${v}`));
  console.error("");
  console.error(`Template: ${policy.template || "desconhecido"}`);
  console.error(`Versao: ${policy.version || "desconhecida"}`);
  console.error("");
  
  if (policy.version === "free") {
    console.error("Dica: Se precisar de componentes Pro, recrie o projeto com -Version pro");
    console.error("");
  }
  
  process.exit(1);
}

console.log("");
console.log("✅ Policy gate passou.");
console.log(`   Report: ${path.relative(process.cwd(), reportPath)}`);
console.log(`   Template: ${policy.template || "desconhecido"}`);
console.log(`   Versao: ${policy.version || "desconhecida"}`);
if (scanImports) {
  console.log(`   Imports escaneados: OK`);
}
console.log("");

// ==============================
// Funcoes Auxiliares
// ==============================

/**
 * Escaneia arquivos em busca de imports proibidos
 */
function scanForForbiddenImports(dir, forbiddenImports) {
  const results = [];
  const extensions = [".js", ".jsx", ".ts", ".tsx"];
  
  function walkDir(currentDir) {
    let files;
    try {
      files = fs.readdirSync(currentDir);
    } catch (err) {
      return; // Ignorar erros de leitura
    }
    
    for (const file of files) {
      const filePath = path.join(currentDir, file);
      let stat;
      try {
        stat = fs.statSync(filePath);
      } catch (err) {
        continue; // Ignorar arquivos inacessiveis
      }
      
      if (stat.isDirectory()) {
        // Ignorar node_modules, pastas ocultas e examples
        if (!file.startsWith(".") && file !== "node_modules" && file !== "examples") {
          walkDir(filePath);
        }
      } else if (extensions.some(ext => file.endsWith(ext))) {
        const content = fs.readFileSync(filePath, "utf-8");
        const lines = content.split("\n");
        
        for (const forbidden of forbiddenImports) {
          // Procurar em cada linha para reportar numero da linha
          lines.forEach((lineContent, index) => {
            const importRegex = new RegExp(
              `(import\\s+.*?from\\s+['"]${escapeRegex(forbidden)}['"]|` +
              `require\\s*\\(\\s*['"]${escapeRegex(forbidden)}['"]\\s*\\))`,
              "g"
            );
            
            if (importRegex.test(lineContent)) {
              const relativePath = path.relative(process.cwd(), filePath);
              // Evitar duplicatas
              const exists = results.some(r => r.file === relativePath && r.importName === forbidden);
              if (!exists) {
                results.push({ 
                  file: relativePath, 
                  importName: forbidden,
                  line: index + 1
                });
              }
            }
          });
        }
      }
    }
  }
  
  walkDir(dir);
  return results;
}

/**
 * Escapa caracteres especiais de regex
 */
function escapeRegex(string) {
  return string.replace(/[.*+?^${}()|[\]\\]/g, "\\$&");
}

function resolveReportPath(cliArg) {
  if (cliArg) {
    return path.resolve(process.cwd(), cliArg);
  }

  const reportsDir = path.join(process.cwd(), "reports");
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
