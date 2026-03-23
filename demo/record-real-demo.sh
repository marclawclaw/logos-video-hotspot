#!/usr/bin/env bash
set -e

REPO_DIR="/home/marclaw/src/marclawclaw/logos-video-hotspot"
BINARY="$REPO_DIR/build/ui/app/video-hotspot-app"
DEMO_DIR="$REPO_DIR/demo"
OUTPUT="$DEMO_DIR/gui-demo-new.mp4"
DISPLAY_NUM=":99"
WIDTH=1280
HEIGHT=800
FPS=15
DURATION=45

echo "=== Real Live Demo Recorder ==="
echo "Binary: $BINARY"
echo "Output: $OUTPUT"

if [ ! -f "$BINARY" ]; then
    echo "ERROR: Binary not found. Build first:"
    echo "  cmake -B build -DBUILD_UI_APP=ON -DBUILD_UI_PLUGIN=ON"
    echo "  cmake --build build --target video-hotspot-app"
    exit 1
fi

# Clean stale processes
pkill -f "video-hotspot-app" 2>/dev/null || true
sleep 1

# Start Xvfb if not already running
if ! pgrep -f "Xvfb $DISPLAY_NUM" > /dev/null; then
    Xvfb $DISPLAY_NUM -screen 0 "${WIDTH}x${HEIGHT}x24" -ac +extension GLX +render -noreset &
    sleep 2
fi

# Start ffmpeg recording first (captures from X11)
rm -f "$OUTPUT"
DISPLAY=$DISPLAY_NUM ffmpeg -y \
    -f x11grab \
    -framerate $FPS \
    -draw_mouse 0 \
    -i "$DISPLAY_NUM" \
    -c:v libx264 \
    -preset ultrafast \
    -crf 23 \
    -pix_fmt yuv420p \
    -t $DURATION \
    "$OUTPUT" &
FFMPEG_PID=$!
echo "Recording started (ffmpeg pid: $FFMPEG_PID)"

# Wait a moment for ffmpeg to initialize
sleep 1

# Start the app
export DISPLAY=$DISPLAY_NUM
export QT_QPA_PLATFORM=xcb
export QT_QUICK_BACKEND=software
"$BINARY" &
APP_PID=$!
echo "App started (pid: $APP_PID)"

# Wait for recording to finish
wait $FFMPEG_PID
FFMPEG_EXIT=$?
echo "Recording finished (exit code: $FFMPEG_EXIT})"

# Kill app if still running
kill $APP_PID 2>/dev/null || true

if [ -f "$OUTPUT" ] && [ -s "$OUTPUT" ]; then
    echo "SUCCESS: Demo saved to $OUTPUT"
    ls -lh "$OUTPUT"
else
    echo "ERROR: Output file missing or empty"
    exit 1
fi
