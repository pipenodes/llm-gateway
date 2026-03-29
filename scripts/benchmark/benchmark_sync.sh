#!/bin/bash
# HERMES - Benchmark Sincrono (parametrizado)
# Chama POST /v1/benchmark e exibe o resultado.
#
# Uso: benchmark_sync.sh [GATEWAY_URL] [API_KEY] [MODE]
# MODE: full | basic | general | python | agent | general_basic | general_medium | general_advanced |
#       python_basic | python_medium | python_advanced | agent_basic | agent_medium | agent_advanced
#
# Ex:  benchmark_sync.sh http://localhost:8080
#      benchmark_sync.sh http://localhost:8080 "" full
#      benchmark_sync.sh http://localhost:8080 "" general_basic

set -e

GATEWAY_URL="${1:-http://localhost:8080}"
API_KEY="${2:-$API_KEY}"
MODE="${3:-full}"

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
RUNNER="${SCRIPT_DIR}/benchmark_runner.py"
PROMPTS_FILE="${SCRIPT_DIR}/benchmark_prompts.json"
TOOLS_FILE="${SCRIPT_DIR}/benchmark_tools.json"

GENERAL_MODELS='[{"model":"gemma3:1b","provider":"ollama"},{"model":"gemma3:4b","provider":"ollama"},{"model":"gemma3:12b","provider":"ollama"}]'
EVALUATOR='{"model":"gemma3:4b","provider":"ollama"}'
PARAMS='{"temperature":0.7,"max_tokens":256}'

echo "========================================"
echo "  HERMES - Benchmark Sincrono"
echo "========================================"
echo " Target: $GATEWAY_URL"
echo " Mode:   $MODE"
echo "========================================"

echo ""
echo "[1/4] Verificando gateway..."
HEALTH=$(curl -s "$GATEWAY_URL/health")
if [ "$HEALTH" != '{"status":"ok"}' ]; then
    echo "ERRO: Gateway inacessivel em $GATEWAY_URL"
    exit 1
fi
echo "OK"

# Resolve categorias
CATEGORIES=$(python3 "$RUNNER" categories "$MODE" "$PROMPTS_FILE") || {
    echo "ERRO: MODE invalido. Use: full, basic, general, python, agent, ou categoria especifica."
    exit 1
}

echo ""
echo "[2/4] Executando benchmark..."

TMP_RESULTS=$(mktemp)
echo "[]" > "$TMP_RESULTS"
LAST_SUMMARY="{}"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
OUTPUT_DIR="${BENCHMARK_OUTPUT_DIR:-$PROJECT_ROOT}"

for CAT in $CATEGORIES; do
    GROUP="${CAT%_*}"
    SUB="${CAT#*_}"
    echo "  Categoria: $CAT ..."

    if [ "$GROUP" = "agent" ]; then
        MODELS=$(curl -s "$GATEWAY_URL/v1/models" 2>/dev/null | python3 "$RUNNER" agent_models)
    else
        MODELS="$GENERAL_MODELS"
    fi

    USE_TOOLS="0"
    [ "$GROUP" = "agent" ] && USE_TOOLS="1"

    BENCHMARK_BODY=$(python3 "$RUNNER" build_body "$GROUP" "$SUB" "$PROMPTS_FILE" "$TOOLS_FILE" "$USE_TOOLS" "$MODELS" "$PARAMS" "$EVALUATOR")

    TMP_OUT=$(mktemp)
    if [ -n "$API_KEY" ]; then
        HTTP_CODE=$(curl -s -w "%{http_code}" -o "$TMP_OUT" --max-time 900 -X POST "$GATEWAY_URL/v1/benchmark" \
            -H "Content-Type: application/json" \
            -H "Authorization: Bearer $API_KEY" \
            -d "$BENCHMARK_BODY")
    else
        HTTP_CODE=$(curl -s -w "%{http_code}" -o "$TMP_OUT" --max-time 900 -X POST "$GATEWAY_URL/v1/benchmark" \
            -H "Content-Type: application/json" \
            -d "$BENCHMARK_BODY")
    fi
    RESP=$(cat "$TMP_OUT")
    rm -f "$TMP_OUT"

    if [ "$HTTP_CODE" = "404" ]; then
        echo "ERRO: Endpoint /v1/benchmark nao encontrado (404)."
        exit 1
    fi

    if echo "$RESP" | grep -q '"error"'; then
        echo "ERRO na categoria $CAT:"
        echo "$RESP" | python3 -m json.tool 2>/dev/null || echo "$RESP"
        exit 1
    fi

    echo "$RESP" | python3 "$RUNNER" merge "$TMP_RESULTS" "$CAT" > "${TMP_RESULTS}.new"
    mv "${TMP_RESULTS}.new" "$TMP_RESULTS"
    LAST_SUMMARY=$(echo "$RESP" | python3 -c "import json,sys; d=json.load(sys.stdin); print(json.dumps(d.get('summary',{})))" 2>/dev/null)
done

# Monta resposta final (usa arquivo para evitar "Argument list too long")
TMP_FINAL=$(mktemp)
python3 "$RUNNER" final "$TMP_RESULTS" "$LAST_SUMMARY" > "$TMP_FINAL"
rm -f "$TMP_RESULTS"

OUT_PREFIX="${OUTPUT_DIR}/benchmark_${MODE}_${TIMESTAMP}"
mv "$TMP_FINAL" "${OUT_PREFIX}.json"

echo ""
echo "[3/4] Resultado:"
echo ""
python3 -m json.tool < "${OUT_PREFIX}.json" 2>/dev/null || cat "${OUT_PREFIX}.json"

echo ""
echo "[4/4] Salvando resultados..."
echo "  JSON: ${OUT_PREFIX}.json"

python3 "$RUNNER" save_md "${OUT_PREFIX}.md" "$TIMESTAMP" "$MODE" < "${OUT_PREFIX}.json" 2>/dev/null && echo "  MD:   ${OUT_PREFIX}.md" || true

python3 "$RUNNER" save_csv "${OUT_PREFIX}.csv" < "${OUT_PREFIX}.json" 2>/dev/null && echo "  CSV:  ${OUT_PREFIX}.csv" || true

echo ""
echo "========================================"
echo "  Resumo"
echo "========================================"
python3 "$RUNNER" summary < "${OUT_PREFIX}.json" 2>/dev/null || true
echo "========================================"
