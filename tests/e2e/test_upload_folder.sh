#!/usr/bin/env bash
# E2E Test: Upload folder
# FURPS F: "Import all videos from a folder (folder picker)"

set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CLI="${1:-video-hotspot}"
source "${SCRIPT_DIR}/e2e_helpers.sh"

echo "=== E2E: Upload Folder ==="

TMPDIR_TEST="$(mktemp -d)"
trap "rm -rf $TMPDIR_TEST" EXIT

VIDEO_DIR="${TMPDIR_TEST}/videos"
mkdir -p "$VIDEO_DIR"

# Create 3 test videos
for i in 1 2 3; do
    make_test_video "${VIDEO_DIR}/video_${i}.mp4" 512 > /dev/null
done

# Create a non-video file (should be ignored)
echo "not a video" > "${VIDEO_DIR}/readme.txt"

# --- Test 1: upload-folder exits 0
"$CLI" upload-folder "$VIDEO_DIR" > /dev/null 2>&1
EXIT_CODE=$?
if [ "$EXIT_CODE" -eq 0 ]; then
    e2e_pass "upload-folder exits 0"
else
    e2e_fail "upload-folder should exit 0, got $EXIT_CODE"
fi

# --- Test 2: upload-folder returns summary JSON
UF_OUTPUT="$("$CLI" upload-folder "$VIDEO_DIR" 2>&1)"
# Output includes per-file results plus a summary
if echo "$UF_OUTPUT" | grep -q '"status"'; then
    e2e_pass "upload-folder output contains JSON"
else
    e2e_fail "upload-folder should produce JSON output"
fi

# --- Test 3: All 3 videos appear in the index
LIST_OUTPUT="$("$CLI" list 2>&1)"
COUNT="$(echo "$LIST_OUTPUT" | python3 -c "import sys,json; print(json.load(sys.stdin).get('count',0))" 2>/dev/null || echo "0")"
if [ "$COUNT" -ge 3 ]; then
    e2e_pass "all 3 videos appear in index after folder upload (count=$COUNT)"
else
    e2e_fail "upload-folder should result in at least 3 indexed videos (got $COUNT)"
fi

# --- Test 4: Empty folder returns 0 exit and message
EMPTY_DIR="${TMPDIR_TEST}/empty"
mkdir -p "$EMPTY_DIR"
"$CLI" upload-folder "$EMPTY_DIR" > /dev/null 2>&1
EMPTY_EXIT=$?
if [ "$EMPTY_EXIT" -eq 0 ]; then
    e2e_pass "upload-folder on empty dir exits 0"
else
    e2e_fail "upload-folder on empty dir should exit 0, got $EMPTY_EXIT"
fi

# --- Test 5: Non-existent folder returns non-zero exit
"$CLI" upload-folder "/nonexistent/folder" > /dev/null 2>&1 && BAD_EXIT=0 || BAD_EXIT=$?
if [ "$BAD_EXIT" -ne 0 ]; then
    e2e_pass "upload-folder non-existent folder returns non-zero"
else
    e2e_fail "upload-folder non-existent folder should return non-zero"
fi

e2e_summary
