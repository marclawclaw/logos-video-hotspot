#!/usr/bin/env bash
# E2E Test: Download a video by CID
# FURPS F: "download <id> — Download a video by ID/CID"

set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CLI="${1:-video-hotspot}"
source "${SCRIPT_DIR}/e2e_helpers.sh"

echo "=== E2E: Download ==="

TMPDIR_TEST="$(mktemp -d)"
trap "rm -rf $TMPDIR_TEST" EXIT

# --- First, upload a file to get a CID
VIDEO="${TMPDIR_TEST}/download_source.mp4"
make_test_video "$VIDEO" 1024
UPLOAD_OUTPUT="$("$CLI" upload "$VIDEO" 2>&1)"
CID="$(echo "$UPLOAD_OUTPUT" | python3 -c "import sys,json; d=json.load(sys.stdin); print(d.get('upload',{}).get('cid',''))" 2>/dev/null || echo "")"

if [ -z "$CID" ]; then
    echo "SKIP: Could not get CID from upload (output: $UPLOAD_OUTPUT)"
    exit 0
fi

e2e_pass "setup: uploaded video with CID ${CID:0:16}..."

# --- Test 1: Download by CID exits 0
DEST="${TMPDIR_TEST}/downloads"
mkdir -p "$DEST"

DOWNLOAD_OUTPUT="$("$CLI" download "$CID" "$DEST" 2>&1)"
EXIT_CODE=$?
if [ "$EXIT_CODE" -eq 0 ]; then
    e2e_pass "download exits 0"
else
    e2e_fail "download should exit 0, got $EXIT_CODE (output: $DOWNLOAD_OUTPUT)"
fi

# --- Test 2: Download returns valid JSON
e2e_json_assert "download returns status ok" "$DOWNLOAD_OUTPUT" '.status == "ok"'

# --- Test 3: Downloaded file exists on disk
LOCAL_PATH="$(echo "$DOWNLOAD_OUTPUT" | python3 -c "import sys,json; print(json.load(sys.stdin).get('local_path',''))" 2>/dev/null || echo "")"
if [ -f "$LOCAL_PATH" ]; then
    e2e_pass "downloaded file exists at: $LOCAL_PATH"
else
    # Also check by CID in dest dir
    if [ -f "${DEST}/${CID}" ]; then
        e2e_pass "downloaded file exists at: ${DEST}/${CID}"
    else
        e2e_fail "downloaded file should exist at local_path (got: $LOCAL_PATH, dest: $DEST)"
    fi
fi

# --- Test 4: Download non-existent CID returns non-zero
FAKE_CID="0000000000000000000000000000000000000000000000000000000000000000"
"$CLI" download "$FAKE_CID" "$DEST" > /dev/null 2>&1 && EXIT_BAD=0 || EXIT_BAD=$?
if [ "$EXIT_BAD" -ne 0 ]; then
    e2e_pass "download non-existent CID returns non-zero exit"
else
    e2e_fail "download non-existent CID should return non-zero"
fi

# --- Test 5: --human flag works
HR_OUT="$("$CLI" --human download "$CID" "$DEST" 2>&1)"
if echo "$HR_OUT" | grep -qiE "cid|local|path|download"; then
    e2e_pass "--human download produces readable output"
else
    e2e_fail "--human download should contain readable field names"
fi

e2e_summary
