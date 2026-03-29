#!/bin/bash

echo "Testing HERMES - OpenAI Compatibility"
echo "============================================"

GATEWAY_URL="${1:-http://localhost:8080}"
PASS=0
FAIL=0

check() {
    local name="$1" code="$2" expected="$3" body="$4"
    if [ "$code" = "$expected" ]; then
        echo "  [PASS] $name (HTTP $code)"
        ((PASS++))
    else
        echo "  [FAIL] $name (expected $expected, got $code)"
        [ -n "$body" ] && echo "         $body" | head -c 200
        ((FAIL++))
    fi
}

echo ""
echo "[1] Health check..."
RESP=$(curl -s -w "\n%{http_code}" "$GATEWAY_URL/health")
CODE=$(echo "$RESP" | tail -1)
BODY=$(echo "$RESP" | head -1)
check "GET /health" "$CODE" "200"

echo ""
echo "[2] X-Request-Id header..."
HEADER=$(curl -s -o /dev/null -w "%{header_json}" "$GATEWAY_URL/health" 2>/dev/null | grep -o '"x-request-id":[^,}]*' || echo "")
if [ -n "$HEADER" ]; then
    echo "  [PASS] X-Request-Id present"
    ((PASS++))
else
    REQ_ID=$(curl -s -D - -o /dev/null "$GATEWAY_URL/health" 2>/dev/null | grep -i "x-request-id" || echo "")
    if [ -n "$REQ_ID" ]; then
        echo "  [PASS] X-Request-Id present: $REQ_ID"
        ((PASS++))
    else
        echo "  [FAIL] X-Request-Id header not found"
        ((FAIL++))
    fi
fi

echo ""
echo "[3] Client X-Request-Id echo..."
RESP=$(curl -s -D - -o /dev/null "$GATEWAY_URL/health" -H "X-Request-Id: test-req-123" 2>/dev/null)
if echo "$RESP" | grep -qi "test-req-123"; then
    echo "  [PASS] Client X-Request-Id echoed"
    ((PASS++))
else
    echo "  [FAIL] Client X-Request-Id not echoed"
    ((FAIL++))
fi

echo ""
echo "[4] List models (GET /v1/models)..."
RESP=$(curl -s -w "\n%{http_code}" "$GATEWAY_URL/v1/models")
CODE=$(echo "$RESP" | tail -1)
BODY=$(echo "$RESP" | sed '$d')
check "GET /v1/models" "$CODE" "200"
if echo "$BODY" | grep -q '"created"'; then
    echo "  [PASS] Models contain 'created' field"
    ((PASS++))
else
    echo "  [FAIL] Models missing 'created' field"
    ((FAIL++))
fi
if echo "$BODY" | grep -q '"root"'; then
    echo "  [PASS] Models contain 'root' field"
    ((PASS++))
else
    echo "  [FAIL] Models missing 'root' field"
    ((FAIL++))
fi

echo ""
echo "[5] Retrieve model (GET /v1/models/{model})..."
MODEL=$(echo "$BODY" | grep -o '"id":"[^"]*"' | head -1 | cut -d'"' -f4)
if [ -n "$MODEL" ]; then
    RESP=$(curl -s -w "\n%{http_code}" "$GATEWAY_URL/v1/models/$MODEL")
    CODE=$(echo "$RESP" | tail -1)
    check "GET /v1/models/$MODEL" "$CODE" "200"
else
    echo "  [SKIP] No models available"
fi

echo ""
echo "[6] Chat completion (POST /v1/chat/completions)..."
RESP=$(curl -s -w "\n%{http_code}" -X POST "$GATEWAY_URL/v1/chat/completions" \
  -H "Content-Type: application/json" \
  -d '{
    "model": "llama3.2",
    "messages": [{"role": "user", "content": "Say hello in one word."}],
    "temperature": 0.5,
    "frequency_penalty": 0.1,
    "presence_penalty": 0.1,
    "seed": 42,
    "stream": false
  }')
CODE=$(echo "$RESP" | tail -1)
BODY=$(echo "$RESP" | sed '$d')
check "POST /v1/chat/completions" "$CODE" "200"
if echo "$BODY" | grep -q '"system_fingerprint"'; then
    echo "  [PASS] Response contains 'system_fingerprint'"
    ((PASS++))
else
    echo "  [FAIL] Response missing 'system_fingerprint'"
    ((FAIL++))
fi

echo ""
echo "[7] Chat completion - n > 1 rejected..."
RESP=$(curl -s -w "\n%{http_code}" -X POST "$GATEWAY_URL/v1/chat/completions" \
  -H "Content-Type: application/json" \
  -d '{"model": "llama3.2", "messages": [{"role": "user", "content": "Hi"}], "n": 2}')
CODE=$(echo "$RESP" | tail -1)
check "n>1 rejected" "$CODE" "400"

echo ""
echo "[8] Chat completion - JSON mode..."
RESP=$(curl -s -w "\n%{http_code}" -X POST "$GATEWAY_URL/v1/chat/completions" \
  -H "Content-Type: application/json" \
  -d '{
    "model": "llama3.2",
    "messages": [{"role": "user", "content": "Return a JSON object with key greeting and value hello"}],
    "response_format": {"type": "json_object"},
    "stream": false
  }')
CODE=$(echo "$RESP" | tail -1)
check "JSON mode" "$CODE" "200"

echo ""
echo "[9] Text completion (POST /v1/completions)..."
RESP=$(curl -s -w "\n%{http_code}" -X POST "$GATEWAY_URL/v1/completions" \
  -H "Content-Type: application/json" \
  -d '{
    "model": "llama3.2",
    "prompt": "Once upon a time",
    "max_tokens": 20,
    "stream": false
  }')
CODE=$(echo "$RESP" | tail -1)
BODY=$(echo "$RESP" | sed '$d')
check "POST /v1/completions" "$CODE" "200"
if echo "$BODY" | grep -q '"text_completion"'; then
    echo "  [PASS] Response object is 'text_completion'"
    ((PASS++))
else
    echo "  [FAIL] Response object is not 'text_completion'"
    ((FAIL++))
fi
if echo "$BODY" | grep -q '"cmpl-"'; then
    echo "  [PASS] ID has 'cmpl-' prefix"
    ((PASS++))
else
    ID=$(echo "$BODY" | grep -o '"id":"[^"]*"' | head -1)
    if echo "$ID" | grep -q 'cmpl-'; then
        echo "  [PASS] ID has 'cmpl-' prefix"
        ((PASS++))
    else
        echo "  [FAIL] ID missing 'cmpl-' prefix: $ID"
        ((FAIL++))
    fi
fi

echo ""
echo "[10] Embeddings (POST /v1/embeddings)..."
RESP=$(curl -s -w "\n%{http_code}" -X POST "$GATEWAY_URL/v1/embeddings" \
  -H "Content-Type: application/json" \
  -d '{"model": "nomic-embed-text:latest", "input": "test"}')
CODE=$(echo "$RESP" | tail -1)
BODY=$(echo "$RESP" | sed '$d')
check "POST /v1/embeddings" "$CODE" "200"

echo ""
echo "[11] Embeddings - encoding_format validation..."
RESP=$(curl -s -w "\n%{http_code}" -X POST "$GATEWAY_URL/v1/embeddings" \
  -H "Content-Type: application/json" \
  -d '{"model": "nomic-embed-text:latest", "input": "test", "encoding_format": "base64"}')
CODE=$(echo "$RESP" | tail -1)
check "encoding_format base64 rejected" "$CODE" "400"

echo ""
echo "[12] Model not found (GET /v1/models/nonexistent-model-xyz)..."
RESP=$(curl -s -w "\n%{http_code}" "$GATEWAY_URL/v1/models/nonexistent-model-xyz")
CODE=$(echo "$RESP" | tail -1)
check "Model not found" "$CODE" "404"

echo ""
echo "============================================"
echo "Results: $PASS passed, $FAIL failed"
echo "============================================"

exit $FAIL
