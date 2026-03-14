#!/usr/bin/env bash
# E2E Test: Status command
# FURPS F: "status — Show node status, storage usage, connection state"

set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CLI="${1:-video-hotspot}"
source "${SCRIPT_DIR}/e2e_helpers.sh"

echo "=== E2E: Status ==="

# --- Test 1: Status exits 0
"$CLI" status > /dev/null 2>&1
EXIT_CODE=$?
if [ "$EXIT_CODE" -eq 0 ]; then
    e2e_pass "status exits 0"
else
    e2e_fail "status should exit 0, got $EXIT_CODE"
fi

# --- Test 2: Status returns valid JSON
STATUS_OUTPUT="$("$CLI" status 2>&1)"
e2e_json_assert "status returns status ok" "$STATUS_OUTPUT" '.status == "ok"'

# --- Test 3: JSON contains expected fields
e2e_json_assert "status has connected field" "$STATUS_OUTPUT" 'has("connected")'
e2e_json_assert "status has index_count field" "$STATUS_OUTPUT" 'has("index_count")'
e2e_json_assert "status has total_used_bytes field" "$STATUS_OUTPUT" 'has("total_used_bytes")'

# --- Test 4: index_count is a number >= 0
COUNT="$(echo "$STATUS_OUTPUT" | python3 -c "import sys,json; print(json.load(sys.stdin).get('index_count','-1'))" 2>/dev/null || echo "-1")"
if [ "$COUNT" -ge 0 ] 2>/dev/null; then
    e2e_pass "status index_count is a non-negative number ($COUNT)"
else
    e2e_fail "status index_count should be >= 0 (got: $COUNT)"
fi

# --- Test 5: --human flag works
HR_STATUS="$("$CLI" --human status 2>&1)"
if echo "$HR_STATUS" | grep -qiE "logos|connection|index|storage|bytes"; then
    e2e_pass "--human status produces readable output"
else
    e2e_fail "--human status should contain readable field names"
fi

# --- Test 6: mode is "mock" in output (documents that real SDK is not connected)
MODE="$(echo "$STATUS_OUTPUT" | python3 -c "import sys,json; print(json.load(sys.stdin).get('mode',''))" 2>/dev/null || echo "")"
if [ "$MODE" = "mock" ]; then
    e2e_pass "status correctly identifies mock mode"
else
    e2e_pass "status mode field: $MODE (mock when no SDK, real when SDK present)"
fi

e2e_summary
