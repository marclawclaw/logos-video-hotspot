#!/usr/bin/env bash
# E2E Test: Upload a single video file
# FURPS F: "Import individual video files via file picker"

set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CLI="${1:-video-hotspot}"
source "${SCRIPT_DIR}/e2e_helpers.sh"

echo "=== E2E: Upload ==="

# Create test video
TMPDIR_TEST="$(mktemp -d)"
trap "rm -rf $TMPDIR_TEST" EXIT

VIDEO="${TMPDIR_TEST}/test_video.mp4"
make_test_video "$VIDEO" 2048

# --- Test 1: Upload returns JSON with status=ok
OUTPUT="$("$CLI" upload "$VIDEO" 2>&1)"
e2e_json_assert "upload returns status ok" "$OUTPUT" '.status == "ok" or .upload.status == 4'

# --- Test 2: CID is non-empty
CID="$(echo "$OUTPUT" | python3 -c "import sys,json; d=json.load(sys.stdin); print(d.get('upload',d).get('cid',''))" 2>/dev/null || true)"
if [ -n "$CID" ]; then
    e2e_pass "upload returns non-empty CID: $CID"
else
    e2e_fail "upload should return a non-empty CID (got: $OUTPUT)"
fi

# --- Test 3: Upload exits 0
"$CLI" upload "$VIDEO" > /dev/null 2>&1
EXIT_CODE=$?
# Second upload will be a duplicate — both 0 (dup is ok) and non-zero are acceptable
# Duplicate exits 0
if [ "$EXIT_CODE" -eq 0 ]; then
    e2e_pass "upload exits 0 on success/duplicate"
else
    e2e_fail "upload should exit 0, got $EXIT_CODE"
fi

# --- Test 4: Upload non-existent file returns non-zero exit
"$CLI" upload "/nonexistent/path.mp4" > /dev/null 2>&1 && EXIT_CODE=0 || EXIT_CODE=$?
if [ "$EXIT_CODE" -ne 0 ]; then
    e2e_pass "upload missing file returns non-zero exit"
else
    e2e_fail "upload missing file should return non-zero exit"
fi

# --- Test 5: Human-readable output with --human flag
HR_OUTPUT="$("$CLI" --human upload "$VIDEO" 2>&1)"
if echo "$HR_OUTPUT" | grep -qiE "uploaded|duplicate|cid"; then
    e2e_pass "--human flag produces readable output"
else
    e2e_fail "--human output should contain 'Uploaded' or 'Duplicate' or 'CID'"
fi

e2e_summary
