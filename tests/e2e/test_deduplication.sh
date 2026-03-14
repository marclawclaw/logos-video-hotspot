#!/usr/bin/env bash
# E2E Test: Deduplication — upload same file twice, second should be rejected
# FURPS F: "Deduplication: compute content hash before upload — never upload the same file twice"

set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CLI="${1:-video-hotspot}"
source "${SCRIPT_DIR}/e2e_helpers.sh"

echo "=== E2E: Deduplication ==="

TMPDIR_TEST="$(mktemp -d)"
trap "rm -rf $TMPDIR_TEST" EXIT

VIDEO="${TMPDIR_TEST}/dedup_test.mp4"
make_test_video "$VIDEO" 1024

# --- Test 1: First upload succeeds
OUTPUT1="$("$CLI" upload "$VIDEO" 2>&1)"
e2e_json_assert "first upload returns status ok" "$OUTPUT1" '.status == "ok"'

# Extract CID from first upload
CID1="$(echo "$OUTPUT1" | python3 -c "import sys,json; d=json.load(sys.stdin); print(d.get('upload',{}).get('cid',''))" 2>/dev/null || echo "")"
if [ -n "$CID1" ]; then
    e2e_pass "first upload returns CID: ${CID1:0:16}..."
else
    e2e_fail "first upload should return a CID"
fi

# --- Test 2: Second upload of the same file is a duplicate
OUTPUT2="$("$CLI" upload "$VIDEO" 2>&1)"
e2e_json_assert "second upload returns duplicate status" "$OUTPUT2" '.status == "duplicate"'

# --- Test 3: CID from second upload matches first
CID2="$(echo "$OUTPUT2" | python3 -c "import sys,json; d=json.load(sys.stdin); print(d.get('upload',{}).get('cid',''))" 2>/dev/null || echo "")"
if [ "$CID1" = "$CID2" ] && [ -n "$CID1" ]; then
    e2e_pass "duplicate upload returns same CID as original"
else
    e2e_fail "duplicate upload should return same CID (got first=$CID1, second=$CID2)"
fi

# --- Test 4: Different file is NOT a duplicate
VIDEO2="${TMPDIR_TEST}/different_video.mp4"
make_test_video "$VIDEO2" 2048
OUTPUT3="$("$CLI" upload "$VIDEO2" 2>&1)"
e2e_json_assert "different file is not a duplicate" "$OUTPUT3" '.status == "ok"'

CID3="$(echo "$OUTPUT3" | python3 -c "import sys,json; d=json.load(sys.stdin); print(d.get('upload',{}).get('cid',''))" 2>/dev/null || echo "")"
if [ "$CID1" != "$CID3" ] && [ -n "$CID3" ]; then
    e2e_pass "different file gets a different CID"
else
    e2e_fail "different files should produce different CIDs (got $CID1 vs $CID3)"
fi

# --- Test 5: Both uploads exit 0 (duplicate is not an error)
EXIT_DUP="$("$CLI" upload "$VIDEO" > /dev/null 2>&1; echo $?)"
if [ "$EXIT_DUP" -eq 0 ]; then
    e2e_pass "duplicate upload exits 0 (not an error)"
else
    e2e_fail "duplicate upload should exit 0, got $EXIT_DUP"
fi

e2e_summary
