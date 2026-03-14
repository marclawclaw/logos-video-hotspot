#!/usr/bin/env bash
# demo/record-cli.sh — drives the CLI demo session recorded by asciinema
# Suppress Qt/SQL thread warnings (stderr) so only meaningful output shows.
set -euo pipefail

REPO="$(cd "$(dirname "$0")/.." && pwd)"
BIN="$REPO/build/cli/video-hotspot"
FIXTURES="$REPO/tests/e2e/fixtures"
DOWNLOAD_DIR="$REPO/demo/downloads"
mkdir -p "$DOWNLOAD_DIR"

# Wipe state so the demo always starts clean
rm -f "$HOME/.local/share/VideoHotspot/video-hotspot/"*.db

# ── Helper: print a prompt line then the command, pause, run it ───────────────
run() {
  echo -e "\n\033[1;32m\$\033[0m \033[1m$*\033[0m"
  sleep 0.6
  "$@" 2>/dev/null
  sleep 0.8
}

# ─────────────────────────────────────────────────────────────────────────────
echo -e "\033[1;36m╔══════════════════════════════════════════╗"
echo -e "║   Video Hotspot — CLI Demo               ║"
echo -e "╚══════════════════════════════════════════╝\033[0m"
sleep 1

# 1. Status — empty node
echo -e "\n\033[90m# 1. Check node status (fresh node)\033[0m"
run "$BIN" status --human

# 2. Upload a single file
echo -e "\n\033[90m# 2. Upload a video file\033[0m"
run "$BIN" upload "$FIXTURES/sample1.mp4" --human

# 3. Upload same file again — dedup kicks in
echo -e "\n\033[90m# 3. Upload same file again — dedup prevents duplicate\033[0m"
run "$BIN" upload "$FIXTURES/sample1.mp4" --human

# 4. Upload a folder
echo -e "\n\033[90m# 4. Upload all videos in a folder\033[0m"
run "$BIN" upload-folder "$FIXTURES/folder-demo" --human

# 5. List indexed videos
echo -e "\n\033[90m# 5. List all indexed videos\033[0m"
run "$BIN" list --human

# 6. Download a video by CID
CID="44da7506d4de4d647af7ebffad3893e8ff7c0cefee50c573fc1660b17f2bc78a"
echo -e "\n\033[90m# 6. Download a video by CID\033[0m"
run "$BIN" download "$CID" "$DOWNLOAD_DIR" --human

# 7. Clear cache
echo -e "\n\033[90m# 7. Clear cached (non-user-owned) videos\033[0m"
run "$BIN" cache clear --human

echo -e "\n\033[1;36m✓ Demo complete\033[0m\n"
