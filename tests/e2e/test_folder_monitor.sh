#!/usr/bin/env bash
# E2E test: folder monitor auto-uploads a new file dropped into the watched folder.

set -euo pipefail

CLI="video-hotspot-cli"
FIXTURE="${BASH_SOURCE%/*}/fixtures/sample.mp4"

echo "=== E2E: Folder Monitor ==="

WATCHDIR=$(mktemp -d)
LOGFILE=$(mktemp)

# Start monitor in background
"$CLI" monitor "$WATCHDIR" --json > "$LOGFILE" 2>&1 &
MONITOR_PID=$!

sleep 2

# Drop a file into the watched folder
cp "$FIXTURE" "$WATCHDIR/test_video.mp4"

# Wait for upload
sleep 10

kill "$MONITOR_PID" 2>/dev/null || true

# Parse log for upload complete event
CID=$(grep '"event":"upload_complete"' "$LOGFILE" | head -1 | jq -r '.cid')

rm -rf "$WATCHDIR" "$LOGFILE"

if [ -z "$CID" ] || [ "$CID" = "null" ]; then
    echo '{"result":"FAIL","reason":"no upload_complete event found in monitor log"}' >&2
    exit 1
fi

echo "{\"result\":\"PASS\",\"cid\":\"$CID\"}"
