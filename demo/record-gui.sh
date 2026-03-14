#!/usr/bin/env bash
# record-gui.sh — Records the Video Hotspot GUI demo with narration
#
# Pipeline:
#   1. Start Xvfb virtual display
#   2. Launch the Qt app on the virtual display
#   3. Record screen to gui-demo-raw.mp4 (no audio)
#   4. Drive the UI with xdotool, timed to the narration script
#   5. Stop recording
#   6. Mux narration audio → gui-demo.mp4
#
# Demo flow (timed to match demo/voiceover.txt):
#   0:00  Map tab — SVG map with 5 video pins visible
#   0:15  Drag timeline slider left — pins fade
#   0:26  Drag slider right — all pins appear
#   0:36  Switch to Upload tab
#   0:41  Upload File × 3 (progress bars)
#   1:02  Upload Folder (3 clips batch-added)
#   1:14  Upload File again → DUPLICATE badge
#   1:27  Switch to Downloads tab
#   1:39  Switch to Settings tab
#   1:57  Back to Map — closing
#   ~2:15 End
#
# Requires: Xvfb, ffmpeg, xdotool, espeak-ng (for gen-voice.sh)
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

echo "=== Video Hotspot GUI Demo Recorder ==="
echo "Binary:    $BINARY"
echo "Raw:       $RAW_OUTPUT"
echo "Narration: $NARRATION"
echo "Output:    $FINAL_OUTPUT"
echo ""

# ── Pre-flight ────────────────────────────────────────────────────────────────
if [ ! -f "$BINARY" ]; then
    echo "ERROR: Binary not found: $BINARY"
    echo "Run: cd build && cmake --build . --target video-hotspot-app"
    exit 1
fi

if [ ! -f "$NARRATION" ]; then
    echo "Narration not found — generating..."
    bash "$DEMO_DIR/gen-voice.sh"
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
# Ensure Qt finds the SVG image format plugin
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
# Timeline:    y=740–800  (60 px strip)
#   Slider track: x=115 → x=1150  (1035 px wide)
#   Handle at slider_pct=0.6 → x = 115 + 0.6*1035 ≈ 736

TAB_Y=60
MAP_TAB_X=160
UPLOAD_TAB_X=480
DL_TAB_X=800
SET_TAB_X=1120

SLIDER_Y=762
SLIDER_X_LEFT=165       # far-left  (~0% = oldest)
SLIDER_X_MID=736        # 60% start position
SLIDER_X_RIGHT=1050     # far-right (~100% = all pins)

BTN_Y=119
UPLOAD_FILE_X=105
UPLOAD_FOLDER_X=283

# ─── Demo sequence — timed to match voiceover.txt (~2:25 total) ──────────────

# 0:00 — Map tab default — lines 01-03 (0–14 s)
echo "--- Scene 1: Map tab — SVG map + pins ---"
sleep 5.0   # line-01 plays (0:00-0:05)
sleep 5.0   # line-02 plays (0:05-0:10)
sleep 4.0   # line-03 plays (0:10-0:14)

# 0:14 — Move to slider for line-04
echo "--- Scene 2: Timeline slider interaction ---"
xdotool mousemove $SLIDER_X_MID $SLIDER_Y
sleep 2.0   # 0:15 line-04 "Use the timeline slider..."

# 0:20 — Drag left slowly (line-05 "Dragging left...")
xdotool mousedown 1
xdotool mousemove $((SLIDER_X_MID - 100)) $SLIDER_Y; sleep 0.5
xdotool mousemove $((SLIDER_X_MID - 250)) $SLIDER_Y; sleep 0.5
xdotool mousemove $((SLIDER_X_MID - 400)) $SLIDER_Y; sleep 0.5
xdotool mousemove $SLIDER_X_LEFT          $SLIDER_Y; sleep 2.5  # hold left for line-05 end

# 0:26 — Drag right slowly (line-06 "Dragging right...")
xdotool mousemove $((SLIDER_X_LEFT + 200)) $SLIDER_Y; sleep 0.4
xdotool mousemove $((SLIDER_X_LEFT + 400)) $SLIDER_Y; sleep 0.4
xdotool mousemove $((SLIDER_X_LEFT + 600)) $SLIDER_Y; sleep 0.4
xdotool mousemove $((SLIDER_X_LEFT + 800)) $SLIDER_Y; sleep 0.4
xdotool mousemove $SLIDER_X_RIGHT          $SLIDER_Y; sleep 2.0  # hold right

# 0:31 — Settle at 60%, release (line-07 "Pins respond...")
xdotool mousemove $SLIDER_X_MID $SLIDER_Y
xdotool mouseup 1
sleep 3.0   # line-07 plays + settle

# 0:36 — Switch to Upload tab (line-08)
echo "--- Scene 3: Upload tab ---"
xdotool mousemove $UPLOAD_TAB_X $TAB_Y; sleep 0.3
xdotool click 1
sleep 4.5   # line-08 plays "let's switch..." + brief pause before click

# 0:41 — Upload File click 1 (line-09 "Click Upload File...")
echo "--- Scene 4: Upload files ---"
xdotool mousemove $UPLOAD_FILE_X $BTN_Y; sleep 0.4
xdotool click 1   # event-footage-001.mp4  — progress bar appears
sleep 7.0         # line-09 + line-10 play while bar fills (0:41→0:53)

# 0:53 — Upload File click 2 (line-11 "Let's add a second file")
xdotool click 1   # protest-march-clip2.mp4
sleep 4.5         # line-11 plays + bar fills (0:53→0:57)

# 0:57 — Upload File click 3 (line-12 "And a third...")
xdotool click 1   # rally-coverage.mp4
sleep 5.0         # line-12 plays + bar fills (0:57→1:02)

# 1:02 — Upload Folder (lines 13-14)
echo "--- Scene 5: Upload folder ---"
xdotool mousemove $UPLOAD_FOLDER_X $BTN_Y; sleep 0.4
xdotool click 1                            # folder-clip-001/002/003.mp4
sleep 12.0        # lines 13-14 play while 3 clips appear + fill (1:02→1:14)

# 1:14 — Duplicate detection (lines 15-16)
echo "--- Scene 6: Duplicate detection ---"
xdotool mousemove $UPLOAD_FILE_X $BTN_Y; sleep 0.4
xdotool click 1   # night-session.mp4
sleep 1.0
xdotool click 1   # field-report-hd.mp4
sleep 1.0
xdotool click 1   # event-footage-001.mp4 → DUPLICATE badge appears
sleep 10.5        # lines 15-16 play + viewer sees DUPLICATE badge (1:14→1:27)

# 1:27 — Downloads tab (lines 17-18)
echo "--- Scene 7: Downloads tab ---"
xdotool mousemove $DL_TAB_X $TAB_Y; sleep 0.3
xdotool click 1
sleep 12.0        # lines 17-18 play (1:27→1:39)

# 1:39 — Settings tab (lines 19-21)
echo "--- Scene 8: Settings tab ---"
xdotool mousemove $SET_TAB_X $TAB_Y; sleep 0.3
xdotool click 1
sleep 18.0        # lines 19-21 play (1:39→1:57)

# 1:57 — Back to Map (lines 22-24)
echo "--- Scene 9: Back to Map ---"
xdotool mousemove $MAP_TAB_X $TAB_Y; sleep 0.3
xdotool click 1
sleep 18.0        # lines 22-24 play + end (1:57→2:15)

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
