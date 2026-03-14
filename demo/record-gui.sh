#!/usr/bin/env bash
# record-gui.sh — Records the Video Hotspot GUI demo
# Requires: Xvfb, ffmpeg, xdotool
set -e

REPO_DIR="$(cd "$(dirname "$0")/.." && pwd)"
BINARY="$REPO_DIR/build/ui/app/video-hotspot-app"
OUTPUT="$REPO_DIR/demo/gui-demo.mp4"
DISPLAY_NUM=":99"
WIDTH=1280
HEIGHT=800

echo "=== Video Hotspot GUI Demo Recorder ==="
echo "Binary: $BINARY"
echo "Output: $OUTPUT"

# Clean up any stale instances
pkill -f "Xvfb $DISPLAY_NUM" 2>/dev/null || true
pkill -f "video-hotspot-app" 2>/dev/null || true
sleep 1

# Start virtual display
Xvfb "$DISPLAY_NUM" -screen 0 "${WIDTH}x${HEIGHT}x24" -ac &
XVFB_PID=$!
echo "Xvfb started (pid: $XVFB_PID)"
sleep 1

# Start the app
export DISPLAY="$DISPLAY_NUM"
export QT_QPA_PLATFORM=xcb
"$BINARY" &
APP_PID=$!
echo "App started (pid: $APP_PID)"
sleep 3

# Verify app is running
if ! kill -0 "$APP_PID" 2>/dev/null; then
    echo "ERROR: App failed to start"
    kill $XVFB_PID 2>/dev/null || true
    exit 1
fi

echo "App is running — starting ffmpeg recording"

# Start recording
ffmpeg -y \
    -f x11grab \
    -video_size "${WIDTH}x${HEIGHT}" \
    -framerate 30 \
    -i "${DISPLAY_NUM}.0" \
    -vcodec libx264 \
    -preset fast \
    -crf 22 \
    -pix_fmt yuv420p \
    "$OUTPUT" &
FFMPEG_PID=$!
echo "ffmpeg recording started (pid: $FFMPEG_PID)"
sleep 1

# ─── Coordinates (verified against screenshot) ────────────────────────────
# Tab bar (y≈49 center): 4 equal tabs across 1280px
TAB_Y=55
UPLOAD_TAB_X=160
MAP_TAB_X=480
DL_TAB_X=800
SET_TAB_X=1120

# Upload screen buttons (y≈133, window-relative — window is at 0,0)
BTN_Y=133
UPLOAD_FILE_X=70
UPLOAD_FOLDER_X=185

# ─── Demo sequence ────────────────────────────────────────────────────────

# 0–2s: Pause on Upload tab — show the initial state
sleep 2

# Hover over Upload File button
echo "Hovering over Upload File button..."
xdotool mousemove $UPLOAD_FILE_X $BTN_Y
sleep 0.5

# Click Upload File — adds event-footage-001.mp4
echo "Click 1: Upload File (adds first file)"
xdotool click 1
sleep 2.5

# Click Upload File again — adds protest-march-clip2.mp4
echo "Click 2: Upload File (adds second file)"
xdotool click 1
sleep 2.5

# Click Upload File again — adds rally-coverage.mp4
echo "Click 3: Upload File (adds third file)"
xdotool click 1
sleep 1

# Click Upload Folder — adds 3 folder clips
echo "Click: Upload Folder (adds batch)"
xdotool mousemove $UPLOAD_FOLDER_X $BTN_Y
sleep 0.3
xdotool click 1
sleep 2

# Click Upload File once more — duplicate! (wraps back to event-footage-001.mp4 after 5 files)
# Files 0-4 added → index 5 → 5%5=0 → event-footage-001.mp4 again = DUPLICATE
echo "Click: Upload File (adds files 4 and 5 to get dedup)"
xdotool mousemove $UPLOAD_FILE_X $BTN_Y
sleep 0.2
xdotool click 1  # rally-coverage.mp4 again? No wait - let me recount
sleep 0.3
# At this point: event-footage-001 (idx0), protest-march (idx1), rally (idx2), folder-001, folder-002, folder-003
# mockFileIndex is now 3 (clicks 1,2,3 used indices 0,1,2 → mockFileIndex=3)
# Another click → night-session.mp4 (idx3)
xdotool click 1  # field-report-hd.mp4 (idx4)
sleep 0.3
xdotool click 1  # event-footage-001.mp4 DUPLICATE (idx5 → 5%5=0)
sleep 1.5
# That last one should show DUPLICATE badge

sleep 1.5

# Switch to Map tab
echo "Switching to Map tab..."
xdotool mousemove $MAP_TAB_X $TAB_Y
sleep 0.3
xdotool click 1
sleep 2.5

# Switch to Downloads tab
echo "Switching to Downloads tab..."
xdotool mousemove $DL_TAB_X $TAB_Y
sleep 0.3
xdotool click 1
sleep 2.5

# Switch to Settings tab
echo "Switching to Settings tab..."
xdotool mousemove $SET_TAB_X $TAB_Y
sleep 0.3
xdotool click 1
sleep 2.5

# Return to Upload tab to show completed uploads with dedup badge
echo "Returning to Upload tab..."
xdotool mousemove $UPLOAD_TAB_X $TAB_Y
sleep 0.3
xdotool click 1
sleep 2.5

# ─── End recording ─────────────────────────────────────────────────────────
echo "Stopping recording..."
kill $FFMPEG_PID 2>/dev/null || true
wait $FFMPEG_PID 2>/dev/null || true

# Stop app and Xvfb
kill $APP_PID 2>/dev/null || true
sleep 0.5
kill $XVFB_PID 2>/dev/null || true

echo ""
echo "=== Recording complete ==="
echo "Output: $OUTPUT"
ls -lh "$OUTPUT" 2>/dev/null || echo "WARNING: Output file missing"
