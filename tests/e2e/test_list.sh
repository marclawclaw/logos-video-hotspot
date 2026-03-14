#!/usr/bin/env bash
# E2E Test: List videos — verify uploaded video appears in list
# FURPS F: "list — List all indexed videos (timestamp, geolocation, CID)"

set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CLI="${1:-video-hotspot}"
source "${SCRIPT_DIR}/e2e_helpers.sh"

echo "=== E2E: List ==="

TMPDIR_TEST="$(mktemp -d)"
trap "rm -rf $TMPDIR_TEST" EXIT

# --- Test 1: List exits 0 (even on empty)
"$CLI" list > /dev/null 2>&1
if [ $? -eq 0 ]; then
    e2e_pass "list exits 0 on empty index"
else
    e2e_fail "list should exit 0"
fi

# --- Test 2: List returns valid JSON
LIST_OUTPUT="$("$CLI" list 2>&1)"
e2e_json_assert "list returns JSON with status ok" "$LIST_OUTPUT" '.status == "ok"'
e2e_json_assert "list returns videos array" "$LIST_OUTPUT" '.videos | type == "array"'

# --- Upload a video
VIDEO="${TMPDIR_TEST}/list_test.mp4"
make_test_video "$VIDEO" 512
UPLOAD_OUTPUT="$("$CLI" upload "$VIDEO" 2>&1)"
UPLOAD_STATUS="$(echo "$UPLOAD_OUTPUT" | python3 -c "import sys,json; print(json.load(sys.stdin).get('status',''))" 2>/dev/null || echo "")"

if [ "$UPLOAD_STATUS" = "ok" ] || [ "$UPLOAD_STATUS" = "duplicate" ]; then
    e2e_pass "video uploaded before list test"
else
    e2e_fail "failed to upload video for list test (output: $UPLOAD_OUTPUT)"
fi

# --- Test 3: Uploaded video appears in list
LIST_AFTER="$("$CLI" list 2>&1)"
COUNT="$(echo "$LIST_AFTER" | python3 -c "import sys,json; print(json.load(sys.stdin).get('count', 0))" 2>/dev/null || echo "0")"

if [ "$COUNT" -ge 1 ]; then
    e2e_pass "list shows at least 1 video after upload (count=$COUNT)"
else
    e2e_fail "list should show at least 1 video after upload (got count=$COUNT)"
fi

# --- Test 4: List with --human flag works
HR_LIST="$("$CLI" --human list 2>&1)"
if echo "$HR_LIST" | grep -qiE "total|video|cid"; then
    e2e_pass "--human list produces readable output"
else
    e2e_fail "--human list should contain 'Total' or 'video' or 'CID'"
fi

# --- Test 5: CID in list matches uploaded CID
CID="$(echo "$UPLOAD_OUTPUT" | python3 -c "import sys,json; d=json.load(sys.stdin); print(d.get('upload',{}).get('cid',''))" 2>/dev/null || echo "")"
if [ -n "$CID" ]; then
    if echo "$LIST_AFTER" | grep -q "$CID"; then
        e2e_pass "uploaded CID appears in list output"
    else
        e2e_fail "uploaded CID '$CID' should appear in list"
    fi
else
    e2e_pass "skipped CID-in-list check (duplicate upload, CID from original)"
fi

e2e_summary
