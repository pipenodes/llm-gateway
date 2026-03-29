#!/bin/bash

GATEWAY_URL="${1:-http://localhost:8080}"
REQUESTS="${2:-50}"
CONCURRENCY="${3:-5}"
MODEL="${4:-gemma3:1b}"
API_KEY="${5:-$API_KEY}"

echo "========================================"
echo "  HERMES - Stress Test"
echo "========================================"
echo " Target:      $GATEWAY_URL"
echo " Requests:    $REQUESTS"
echo " Concurrency: $CONCURRENCY"
echo " Model:       $MODEL"
echo "========================================"

echo ""
echo "[1/4] Checking gateway health..."
HEALTH=$(curl -s "$GATEWAY_URL/health")
if [ "$HEALTH" != '{"status":"ok"}' ]; then
    echo "ERROR: Gateway not reachable at $GATEWAY_URL"
    exit 1
fi
echo "OK"

echo ""
echo "[2/4] Capturing initial metrics..."
METRICS_BEFORE=$(curl -s "$GATEWAY_URL/metrics")
echo "$METRICS_BEFORE"
MEM_BEFORE=$(echo "$METRICS_BEFORE" | grep -o '"memory_rss_kb" *: *[0-9]*' | grep -o '[0-9]*$')

echo ""
echo "[3/4] Sending $REQUESTS requests ($CONCURRENCY concurrent)..."

REQUEST_BODY=$(cat <<EOF
{"model":"$MODEL","messages":[{"role":"user","content":"Say hi in one word."}],"stream":false,"max_tokens":5}
EOF
)

TMPDIR_RESULTS=$(mktemp -d)
START_TIME=$(date +%s)

CURL_AUTH=""
if [ -n "$API_KEY" ]; then
    CURL_AUTH="-H \"Authorization: Bearer $API_KEY\""
fi
seq 1 "$REQUESTS" | xargs -P "$CONCURRENCY" -I{} sh -c "
    STATUS=\$(curl -s -o /dev/null -w '%{http_code}' -X POST '$GATEWAY_URL/v1/chat/completions' \
        -H 'Content-Type: application/json' $CURL_AUTH \
        -d '$REQUEST_BODY')
    echo \$STATUS > $TMPDIR_RESULTS/{}.txt
"

END_TIME=$(date +%s)
ELAPSED=$((END_TIME - START_TIME))

SUCCESS=$(grep -rl '200' "$TMPDIR_RESULTS" | wc -l)
ERRORS=$((REQUESTS - SUCCESS))
rm -rf "$TMPDIR_RESULTS"

echo "Done in ${ELAPSED}s -- $SUCCESS OK, $ERRORS errors"
if [ "$ELAPSED" -gt 0 ]; then
    RPS=$(echo "scale=1; $REQUESTS / $ELAPSED" | bc 2>/dev/null || echo "N/A")
    echo "Throughput: ~${RPS} req/s"
fi

echo ""
echo "[4/4] Capturing final metrics..."
METRICS_AFTER=$(curl -s "$GATEWAY_URL/metrics")
echo "$METRICS_AFTER"
MEM_AFTER=$(echo "$METRICS_AFTER" | grep -o '"memory_rss_kb" *: *[0-9]*' | grep -o '[0-9]*$')

echo ""
echo "========================================"
echo "  Results"
echo "========================================"
if [ -n "$MEM_BEFORE" ] && [ -n "$MEM_AFTER" ]; then
    MEM_DELTA=$((MEM_AFTER - MEM_BEFORE))
    echo " Memory before: ${MEM_BEFORE} KB"
    echo " Memory after:  ${MEM_AFTER} KB"
    echo " Memory delta:  ${MEM_DELTA} KB"
    if [ "$MEM_DELTA" -gt 10240 ]; then
        echo " WARNING: Memory grew by more than 10 MB -- possible leak"
    else
        echo " OK: Memory growth within normal range"
    fi
else
    echo " (Could not read memory metrics -- /proc not available?)"
fi
echo "========================================"
