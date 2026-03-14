#!/usr/bin/env bash
# record-gui.sh — Records the Video Hotspot GUI demo
# Tab order: Map (first) → Upload → Downloads → Settings
# Demo flow:
#   1. Open on Map tab — show pins on canvas map
#   2. Drag timeline slider left/right — pins respond with opacity fade
#   3. Switch to Upload → add files + folder → duplicate badge
#   4. Switch to Downloads, Settings, back to Map
#
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
pkill -f "video-hotspot-app"  2>/dev/null || true
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

# ─── Coordinate reference (1280×800 window) ──────────────────────────────────
#
# Title bar:  y=0–40
# Tab bar:    y=40–80    (4 equal tabs, each 320 px wide)
#   Map       centre x=160,  y=60
#   Upload    centre x=480,  y=60
#   Downloads centre x=800,  y=60
#   Settings  centre x=1120, y=60
#
# Map content: y=80–740   (660 px tall map area)
# Timeline:    y=740–800  (60 px strip)
#   "Timeline:" label ends ~x=105
#   Slider track: x=110 → x=1150  (1040 px wide)
#   Handle at slider_pct=0.6 → x = 110 + 0.6*1040 = 734
#
# Upload buttons (y≈115 centre):
#   Upload File   centre x=104, y=115
#   Upload Folder centre x=280, y=115

TAB_Y=60
MAP_TAB_X=160
UPLOAD_TAB_X=480
DL_TAB_X=800
SET_TAB_X=1120

SLIDER_Y=762           # vertical centre of slider handle
SLIDER_X_LEFT=160      # drag target: far-left (shows ~1 pin)
SLIDER_X_MID=734       # initial handle position (60%)
SLIDER_X_RIGHT=1040    # drag target: far-right (shows all pins)

BTN_Y=115
UPLOAD_FILE_X=104
UPLOAD_FOLDER_X=280

# ─── Demo sequence ────────────────────────────────────────────────────────────

# 1 — Land on Map tab (default), pause to show the canvas map + pins
echo "--- Scene 1: Map tab with pins ---"
sleep 2.5

# 2 — Move mouse to slider handle
echo "--- Scene 2: Move to slider handle ---"
xdotool mousemove $SLIDER_X_MID $SLIDER_Y
sleep 0.6

# 3 — Drag slider all the way LEFT (few pins visible)
# Use instant moves + sleep pauses so viewer sees intermediate positions.
# The QML NumberAnimation on pin opacity plays smoothly in between.
echo "--- Scene 3: Drag slider left (fade most pins) ---"
xdotool mousedown 1
sleep 0.1
xdotool mousemove $((SLIDER_X_MID - 150)) $SLIDER_Y; sleep 0.4
xdotool mousemove $((SLIDER_X_MID - 300)) $SLIDER_Y; sleep 0.4
xdotool mousemove $SLIDER_X_LEFT          $SLIDER_Y; sleep 0.8

# 4 — Drag slider all the way RIGHT (all pins visible)
echo "--- Scene 4: Drag slider right (all pins appear) ---"
xdotool mousemove $((SLIDER_X_LEFT + 300)) $SLIDER_Y; sleep 0.35
xdotool mousemove $((SLIDER_X_LEFT + 600)) $SLIDER_Y; sleep 0.35
xdotool mousemove $SLIDER_X_RIGHT          $SLIDER_Y; sleep 0.8

# 5 — Release and settle back to 60%
echo "--- Scene 5: Release slider, settle at 60% ---"
xdotool mousemove $SLIDER_X_MID $SLIDER_Y
xdotool mouseup 1
sleep 1.5

# 6 — Switch to Upload tab
echo "--- Scene 6: Upload tab ---"
xdotool mousemove $UPLOAD_TAB_X $TAB_Y
sleep 0.3
xdotool click 1
sleep 2.0

# 7 — Upload files one by one
echo "--- Scene 7: Upload files ---"
xdotool mousemove $UPLOAD_FILE_X $BTN_Y
sleep 0.4
xdotool click 1   # event-footage-001.mp4
sleep 2.0

xdotool click 1   # protest-march-clip2.mp4
sleep 2.0

xdotool click 1   # rally-coverage.mp4
sleep 1.0

# 8 — Upload folder (adds 3 clips)
echo "--- Scene 8: Upload folder ---"
xdotool mousemove $UPLOAD_FOLDER_X $BTN_Y
sleep 0.3
xdotool click 1
sleep 2.0

# 9 — Upload again to trigger DUPLICATE badge
echo "--- Scene 9: Duplicate detection ---"
xdotool mousemove $UPLOAD_FILE_X $BTN_Y
sleep 0.3
xdotool click 1   # night-session.mp4
sleep 0.4
xdotool click 1   # field-report-hd.mp4
sleep 0.4
xdotool click 1   # event-footage-001.mp4 → DUPLICATE
sleep 1.5

# 10 — Downloads tab
echo "--- Scene 10: Downloads tab ---"
xdotool mousemove $DL_TAB_X $TAB_Y
sleep 0.3
xdotool click 1
sleep 2.5

# 11 — Settings tab
echo "--- Scene 11: Settings tab ---"
xdotool mousemove $SET_TAB_X $TAB_Y
sleep 0.3
xdotool click 1
sleep 2.5

# 12 — Return to Map tab
echo "--- Scene 12: Back to Map ---"
xdotool mousemove $MAP_TAB_X $TAB_Y
sleep 0.3
xdotool click 1
sleep 2.5

# ─── End recording ────────────────────────────────────────────────────────────
echo "Stopping recording..."
kill $FFMPEG_PID 2>/dev/null || true
wait $FFMPEG_PID 2>/dev/null || true

kill $APP_PID   2>/dev/null || true
sleep 0.5
kill $XVFB_PID  2>/dev/null || true

echo ""
echo "=== Recording complete ==="
echo "Output: $OUTPUT"
ls -lh "$OUTPUT" 2>/dev/null || echo "WARNING: Output file missing"
