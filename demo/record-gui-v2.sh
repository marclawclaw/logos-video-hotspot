#!/usr/bin/env bash
# record-gui-v2.sh — Records the Video Hotspot GUI demo with:
#   - Natural narration via edge-tts (en-US-AndrewNeural)
#   - Map drag interaction (panning the map)
#   - Timeline slider interaction
#
# Requires: Xvfb, ffmpeg, xdotool, edge-tts
set -e

REPO_DIR="$(cd "$(dirname "$0")/.." && pwd)"
BINARY="$REPO_DIR/build/ui/app/video-hotspot-app"
DEMO_DIR="$REPO_DIR/demo"
RAW_OUTPUT="$DEMO_DIR/gui-demo-raw.mp4"
NARRATION="$DEMO_DIR/voice/narration.wav"
FINAL_OUTPUT="$DEMO_DIR/gui-demo.mp4"
DISPLAY_NUM=":99"
WIDTH=1280
HEIGHT=800

echo "=== Video Hotspot GUI Demo Recorder v2 ==="
echo "Binary:    $BINARY"
echo "Raw:       $RAW_OUTPUT"
echo "Narration: $NARRATION"
echo "Output:    $FINAL_OUTPUT"
echo ""

# ── Pre-flight ────────────────────────────────────────────────────────────────
if [ ! -f "$BINARY" ]; then
    echo "ERROR: Binary not found: $BINARY"
    exit 1
fi

if [ ! -f "$NARRATION" ]; then
    echo "Narration not found — generating with edge-tts..."
    bash "$DEMO_DIR/gen-voice-edge.sh"
fi

# Clean up any stale instances
pkill -f "Xvfb $DISPLAY_NUM" 2>/dev/null || true
pkill -f "video-hotspot-app"  2>/dev/null || true
sleep 1

# ── Start Xvfb ───────────────────────────────────────────────────────────────
Xvfb "$DISPLAY_NUM" -screen 0 "${WIDTH}x${HEIGHT}x24" -ac &
XVFB_PID=$!
echo "Xvfb started (pid: $XVFB_PID)"
sleep 1

# ── Start the app ─────────────────────────────────────────────────────────────
export DISPLAY="$DISPLAY_NUM"
export QT_QPA_PLATFORM=xcb
export QMLSCENE_DEVICE=softwarecontext
export QT_QUICK_BACKEND=software
export QT_PLUGIN_PATH="/usr/lib/aarch64-linux-gnu/qt6/plugins:${QT_PLUGIN_PATH:-}"
"$BINARY" &
APP_PID=$!
echo "App started (pid: $APP_PID)"
sleep 3

if ! kill -0 "$APP_PID" 2>/dev/null; then
    echo "ERROR: App failed to start"
    kill $XVFB_PID 2>/dev/null || true
    exit 1
fi

# ── Start recording (video only, no audio) ────────────────────────────────────
echo "Starting screen recording..."
ffmpeg -y \
    -f x11grab \
    -video_size "${WIDTH}x${HEIGHT}" \
    -framerate 30 \
    -i "${DISPLAY_NUM}.0" \
    -vcodec libx264 \
    -preset fast \
    -crf 20 \
    -pix_fmt yuv420p \
    "$RAW_OUTPUT" &
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
# Map content: y=80–740   (660 px tall SVG map area)
#   Map centre: x=640, y=400
# Timeline:    y=740–800  (60 px strip)
#   Slider track: x=115 → x=1150  (1035 px wide)
#   Handle at slider_pct=0.6 → x = 115 + 0.6*1035 ≈ 736

TAB_Y=60
MAP_TAB_X=160
UPLOAD_TAB_X=480
DL_TAB_X=800
SET_TAB_X=1120

# Map area coordinates for drag interaction
MAP_X_START=400
MAP_Y_START=350
MAP_X_END=700
MAP_Y_END=450

SLIDER_Y=762
SLIDER_X_LEFT=165
SLIDER_X_MID=736
SLIDER_X_RIGHT=1050

BTN_Y=119
UPLOAD_FILE_X=105
UPLOAD_FOLDER_X=283

# ─── Demo sequence ────────────────────────────────────────────────────────────

# 0:00 — Map tab — show SVG map with 5 video pins (lines 01-03, ~14s)
echo "--- Scene 1: Map tab — SVG map + pins ---"
sleep 5.0   # line-01 plays (Welcome to Video Hotspot)
sleep 5.0   # line-02 plays (Map tab shows geotagged...)
sleep 4.0   # line-03 plays (Each coloured pin...)

# ~0:14 — MAP DRAG INTERACTION: pan the map left-to-right
echo "--- Scene 1b: Map drag interaction ---"
# Move cursor to map area
xdotool mousemove $MAP_X_START $MAP_Y_START
sleep 0.3
# Press and drag slowly across the map to simulate panning
xdotool mousedown 1
sleep 0.1
# Drag in increments for smooth animation
for x in $(seq $MAP_X_START 20 $MAP_X_END); do
    xdotool mousemove $x $MAP_Y_START
    sleep 0.04
done
# Continue drag diagonally
for step in 1 2 3 4 5; do
    nx=$((MAP_X_END + step*10))
    ny=$((MAP_Y_START + step*8))
    xdotool mousemove $nx $ny
    sleep 0.05
done
xdotool mouseup 1
sleep 0.5

# Drag back (right to left)
xdotool mousemove $MAP_X_END $MAP_Y_END
sleep 0.2
xdotool mousedown 1
for x in $(seq $MAP_X_END -20 $MAP_X_START); do
    xdotool mousemove $x $MAP_Y_END
    sleep 0.04
done
xdotool mouseup 1
sleep 1.0

# 0:20 — Timeline slider interaction (lines 04-07)
echo "--- Scene 2: Timeline slider interaction ---"
xdotool mousemove $SLIDER_X_MID $SLIDER_Y
sleep 2.0   # line-04 "Use the timeline slider..."

# Drag left slowly (line-05 "Dragging left...")
xdotool mousedown 1
xdotool mousemove $((SLIDER_X_MID - 100)) $SLIDER_Y; sleep 0.5
xdotool mousemove $((SLIDER_X_MID - 250)) $SLIDER_Y; sleep 0.5
xdotool mousemove $((SLIDER_X_MID - 400)) $SLIDER_Y; sleep 0.5
xdotool mousemove $SLIDER_X_LEFT          $SLIDER_Y; sleep 2.5

# Drag right slowly (line-06 "Dragging right...")
xdotool mousemove $((SLIDER_X_LEFT + 200)) $SLIDER_Y; sleep 0.4
xdotool mousemove $((SLIDER_X_LEFT + 400)) $SLIDER_Y; sleep 0.4
xdotool mousemove $((SLIDER_X_LEFT + 600)) $SLIDER_Y; sleep 0.4
xdotool mousemove $((SLIDER_X_LEFT + 800)) $SLIDER_Y; sleep 0.4
xdotool mousemove $SLIDER_X_RIGHT          $SLIDER_Y; sleep 2.0

# Settle at 60%, release (line-07 "Pins respond...")
xdotool mousemove $SLIDER_X_MID $SLIDER_Y
xdotool mouseup 1
sleep 3.0

# 0:36 — Switch to Upload tab (line-08)
echo "--- Scene 3: Upload tab ---"
xdotool mousemove $UPLOAD_TAB_X $TAB_Y; sleep 0.3
xdotool click 1
sleep 4.5

# 0:41 — Upload files (lines 09-12)
echo "--- Scene 4: Upload files ---"
xdotool mousemove $UPLOAD_FILE_X $BTN_Y; sleep 0.4
xdotool click 1
sleep 7.0

xdotool click 1
sleep 4.5

xdotool click 1
sleep 5.0

# 1:02 — Upload Folder (lines 13-14)
echo "--- Scene 5: Upload folder ---"
xdotool mousemove $UPLOAD_FOLDER_X $BTN_Y; sleep 0.4
xdotool click 1
sleep 12.0

# 1:14 — Duplicate detection (lines 15-16)
echo "--- Scene 6: Duplicate detection ---"
xdotool mousemove $UPLOAD_FILE_X $BTN_Y; sleep 0.4
xdotool click 1
sleep 1.0
xdotool click 1
sleep 1.0
xdotool click 1
sleep 10.5

# 1:27 — Downloads tab (lines 17-18)
echo "--- Scene 7: Downloads tab ---"
xdotool mousemove $DL_TAB_X $TAB_Y; sleep 0.3
xdotool click 1
sleep 12.0

# 1:39 — Settings tab (lines 19-21)
echo "--- Scene 8: Settings tab ---"
xdotool mousemove $SET_TAB_X $TAB_Y; sleep 0.3
xdotool click 1
sleep 18.0

# 1:57 — Back to Map (lines 22-24)
echo "--- Scene 9: Back to Map + final map drag ---"
xdotool mousemove $MAP_TAB_X $TAB_Y; sleep 0.3
xdotool click 1
sleep 2.0

# One more map drag to bookend the demo
xdotool mousemove 500 300
sleep 0.2
xdotool mousedown 1
for x in $(seq 500 25 800); do
    xdotool mousemove $x 300
    sleep 0.05
done
xdotool mouseup 1

sleep 16.0   # lines 22-24 play + end

# ─── End recording ────────────────────────────────────────────────────────────
echo "Stopping recording..."
kill $FFMPEG_PID 2>/dev/null || true
wait $FFMPEG_PID 2>/dev/null || true

kill $APP_PID   2>/dev/null || true
sleep 0.5
kill $XVFB_PID  2>/dev/null || true

if [ ! -f "$RAW_OUTPUT" ]; then
    echo "ERROR: Raw recording missing"
    exit 1
fi

echo ""
echo "=== Muxing narration audio ==="
ffmpeg -y \
    -i "$RAW_OUTPUT" \
    -i "$NARRATION" \
    -c:v copy \
    -c:a aac \
    -b:a 128k \
    -shortest \
    "$FINAL_OUTPUT"

echo ""
echo "=== Recording complete ==="
echo "Raw (no audio): $RAW_OUTPUT"
echo "Final:          $FINAL_OUTPUT"
ls -lh "$RAW_OUTPUT" "$FINAL_OUTPUT" 2>/dev/null
