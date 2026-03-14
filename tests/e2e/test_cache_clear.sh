#!/usr/bin/env bash
# E2E Test: Cache clear — clear cached (non-user-owned) videos
# FURPS F: "cache clear — Clear cached (non-user-owned) videos"

set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CLI="${1:-video-hotspot}"
source "${SCRIPT_DIR}/e2e_helpers.sh"

echo "=== E2E: Cache Clear ==="

TMPDIR_TEST="$(mktemp -d)"
trap "rm -rf $TMPDIR_TEST" EXIT

# --- Test 1: cache clear exits 0 even on empty cache
"$CLI" cache clear > /dev/null 2>&1
EXIT_CODE=$?
if [ "$EXIT_CODE" -eq 0 ]; then
    e2e_pass "cache clear exits 0 on empty cache"
else
    e2e_fail "cache clear should exit 0, got $EXIT_CODE"
fi

# --- Test 2: cache clear returns valid JSON
CLEAR_OUTPUT="$("$CLI" cache clear 2>&1)"
e2e_json_assert "cache clear returns status ok" "$CLEAR_OUTPUT" '.status == "ok"'

# --- Test 3: cleared_count is a number >= 0
COUNT="$(echo "$CLEAR_OUTPUT" | python3 -c "import sys,json; print(json.load(sys.stdin).get('cleared_count','-1'))" 2>/dev/null || echo "-1")"
if [ "$COUNT" -ge 0 ] 2>/dev/null; then
    e2e_pass "cache clear returns cleared_count >= 0 (got $COUNT)"
else
    e2e_fail "cache clear should return cleared_count >= 0 (got $COUNT)"
fi

# --- Test 4: cleared_bytes is a number >= 0
BYTES="$(echo "$CLEAR_OUTPUT" | python3 -c "import sys,json; print(json.load(sys.stdin).get('cleared_bytes','-1'))" 2>/dev/null || echo "-1")"
if [ "$BYTES" -ge 0 ] 2>/dev/null; then
    e2e_pass "cache clear returns cleared_bytes >= 0 (got $BYTES)"
else
    e2e_fail "cache clear should return cleared_bytes >= 0 (got $BYTES)"
fi

# --- Test 5: --human flag works
HR_CLEAR="$("$CLI" --human cache clear 2>&1)"
if echo "$HR_CLEAR" | grep -qiE "cache|clear|video|ok"; then
    e2e_pass "--human cache clear produces readable output"
else
    e2e_fail "--human cache clear should produce readable output"
fi

# --- Test 6: status after clear shows reduced/zero cached bytes
STATUS_AFTER="$("$CLI" status 2>&1)"
CACHED="$(echo "$STATUS_AFTER" | python3 -c "import sys,json; print(json.load(sys.stdin).get('cached_bytes','-1'))" 2>/dev/null || echo "-1")"
if [ "$CACHED" -ge 0 ] 2>/dev/null; then
    e2e_pass "status after clear shows cached_bytes >= 0 (got $CACHED)"
else
    e2e_fail "status after clear should show cached_bytes >= 0"
fi

e2e_summary
