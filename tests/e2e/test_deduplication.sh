#!/usr/bin/env bash
# E2E test: uploading the same file twice returns the same CID and skips re-upload.

set -euo pipefail

CLI="video-hotspot-cli"
FIXTURE="${BASH_SOURCE%/*}/fixtures/sample.mp4"

echo "=== E2E: Deduplication ==="

FIRST=$("$CLI" upload "$FIXTURE")
CID1=$(echo "$FIRST" | jq -r '.cid')

SECOND=$("$CLI" upload "$FIXTURE")
CID2=$(echo "$SECOND" | jq -r '.cid')
SKIPPED=$(echo "$SECOND" | jq -r '.skipped')

if [ "$CID1" != "$CID2" ]; then
    echo "{\"result\":\"FAIL\",\"reason\":\"different CIDs on second upload\",\"cid1\":\"$CID1\",\"cid2\":\"$CID2\"}" >&2
    exit 1
fi

if [ "$SKIPPED" != "true" ]; then
    echo "{\"result\":\"FAIL\",\"reason\":\"second upload not marked as skipped\"}" >&2
    exit 1
fi

echo "{\"result\":\"PASS\",\"cid\":\"$CID1\",\"skipped\":true}"
