#!/usr/bin/env bash
# Smoke-tests the coordinator REST API by hitting every endpoint with curl.
# Usage: api-smoke.sh [host-ip]   (default 192.168.1.42)
set -euo pipefail

HOST="${1:-192.168.1.42}"

echo "=== GET /api/status"
curl -fsS "http://$HOST/api/status" | jq .
echo ""

echo "=== GET /api/sensors"
curl -fsS "http://$HOST/api/sensors" | jq .
echo ""

echo "=== GET /api/config"
curl -fsS "http://$HOST/api/config" | jq .
echo ""

echo "=== GET /api/history?metric=soil_moisture&hours=24"
curl -fsS "http://$HOST/api/history?metric=soil_moisture&hours=24" | jq .
echo ""

echo "=== POST /api/pump ON"
curl -fsS -X POST "http://$HOST/api/pump" \
  -H 'content-type: application/json' \
  -d '{"state":"ON"}' | jq .
echo ""

sleep 2

echo "=== POST /api/pump OFF"
curl -fsS -X POST "http://$HOST/api/pump" \
  -H 'content-type: application/json' \
  -d '{"state":"OFF"}' | jq .
echo ""

echo "=== POST /api/config (auto_water)"
curl -fsS -X POST "http://$HOST/api/config" \
  -H 'content-type: application/json' \
  -d '{"auto_water":{"enabled":true,"trigger_below_pct":25,"min_interval_min":60,"duration_s":12}}' | jq .
echo ""

echo "=== Negative test — GET unknown endpoint expect 404"
curl -sS -o /tmp/_resp -w 'HTTP %{http_code}\n' "http://$HOST/api/does-not-exist"
cat /tmp/_resp | jq . || echo "(not JSON, likely 404 HTML)"
echo ""

echo
echo "OK — all endpoints responded successfully."
echo "NOTE: POST /api/system/provisioning would reboot the board — run manually if desired:"
echo "      curl -X POST http://$HOST/api/system/provisioning"
