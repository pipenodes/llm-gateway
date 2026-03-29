#!/bin/bash

echo "HERMES - Docker Compose"
echo "============================="

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="${SCRIPT_DIR}/../.."
cd "$PROJECT_ROOT"

docker compose up -d --build

echo ""
echo "Services started!"
echo "============================="
echo "Gateway: http://localhost:8080"
echo "Ollama:  http://localhost:11434"
echo ""
echo "Pull a model before testing:"
echo "  docker exec ollama ollama pull llama3.2"
echo ""
echo "Test:"
echo "  bash scripts/test/test_gateway.sh http://localhost:8080"
echo ""
echo "Stop:"
echo "  docker compose down"
