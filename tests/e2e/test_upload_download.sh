#!/usr/bin/env bash
# E2E test: upload a video, then download it and verify integrity.
# Requires: LOGOS_TESTNET_ENDPOINT set, video-hotspot-cli in PATH.
# Output: JSON to stdout; exit 0 on pass, non-zero on failure.

set -euo pipefail

CLI="video-hotspot-cli"
FIXTURE="${BASH_SOURCE%/*}/fixtures/sample.mp4"

echo "=== E2E: Upload + Download ==="

# Upload
UPLOAD_OUT=$("$CLI" upload "$FIXTURE")
CID=$(echo "$UPLOAD_OUT" | jq -r '.cid')

if [ -z "$CID" ] || [ "$CID" = "null" ]; then
    echo '{"result":"FAIL","reason":"no CID returned from upload"}' >&2
    exit 1
fi

echo "Uploaded: $CID"

# Download
TMPDIR=$(mktemp -d)
DOWNLOAD_OUT=$("$CLI" download "$CID" --dest "$TMPDIR")
LOCAL_PATH=$(echo "$DOWNLOAD_OUT" | jq -r '.localPath')

if [ ! -f "$LOCAL_PATH" ]; then
    echo '{"result":"FAIL","reason":"downloaded file not found"}' >&2
    exit 1
fi

# Integrity check: compare BLAKE3 hashes
ORIG_HASH=$(b3sum "$FIXTURE" | awk '{print $1}')
DOWN_HASH=$(b3sum "$LOCAL_PATH" | awk '{print $1}')

if [ "$ORIG_HASH" != "$DOWN_HASH" ]; then
    echo "{\"result\":\"FAIL\",\"reason\":\"hash mismatch\",\"expected\":\"$ORIG_HASH\",\"got\":\"$DOWN_HASH\"}" >&2
    exit 1
fi

echo "{\"result\":\"PASS\",\"cid\":\"$CID\"}"
rm -rf "$TMPDIR"
