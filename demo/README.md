# Demo Files

This folder contains recordings and mockups of the Video Hotspot application.
The main README embeds all of these — this file explains how to replay them locally.

---

## CLI Demo

| File | Format | Description |
|------|--------|-------------|
| `cli-demo.cast` | asciinema v2 | Raw terminal recording (replayable) |
| `cli-demo.gif`  | Animated GIF | Rendered for GitHub embedding |

### Replay the cast (live terminal, with timing)

```bash
asciinema play demo/cli-demo.cast
```

Install asciinema if needed:

```bash
pip install asciinema
# or: brew install asciinema
```

### Re-record

```bash
asciinema rec demo/cli-demo.cast \
  --command "bash demo/record-cli.sh" \
  --title "Video Hotspot CLI Demo" \
  --cols 88 --rows 35 \
  --overwrite
```

Then regenerate the GIF:

```bash
agg demo/cli-demo.cast demo/cli-demo.gif --theme monokai --font-size 13 --speed 1.5
```

`agg` binaries: <https://github.com/asciinema/agg/releases>

---

## GUI Mockups

| File | Description |
|------|-------------|
| `gui-upload.svg` | Upload tab — file picker, upload queue, dedup badges |
| `gui-map.svg`    | Map tab — geo pins, expanded video popup, timeline slider |

These are generated SVG screenshots of `VideoHotspotApp.qml` (dark theme).

### Regenerate

```bash
python3 demo/gen-gui-mockup.py
```

### Running the real GUI

Qt6Quick is required. If available:

```bash
cmake -B build -DBUILD_UI_APP=ON
cmake --build build --target video-hotspot-app
./build/ui/app/video-hotspot-app
```

---

## Virtual Display (Xvfb)

When there is no physical display (CI, headless server, Raspberry Pi), use Xvfb to
provide a virtual X11 screen for the Qt app and record it with ffmpeg.

### 1. Install Xvfb

```bash
sudo apt-get install -y xvfb
```

### 2. Start the virtual display

```bash
Xvfb :99 -screen 0 1280x800x24 -nolisten tcp &
export DISPLAY=:99
```

Verify it's running:

```bash
xset q   # should return keyboard/screen settings without error
ls /tmp/.X99-lock   # lock file confirms display :99 is live
```

### 3. Build the app with the UI plugin

```bash
cmake -B build -DBUILD_UI_APP=ON -DBUILD_UI_PLUGIN=ON
cmake --build build --target video_hotspot_plugin video-hotspot-app -j4
```

> **Note:** `BUILD_UI_PLUGIN` must be `ON` so that `createWidget()` returns a real
> `QQuickWidget` rather than `nullptr`. The app `CMakeLists.txt` also needs
> `Qt6::Widgets` and the plugin needs `Qt6::QuickWidgets`.

### 4. Launch the app on the virtual display

```bash
cd /path/to/logos-video-hotspot
DISPLAY=:99 QT_QPA_PLATFORM=xcb LD_LIBRARY_PATH=./build/ui/plugin:$LD_LIBRARY_PATH \
  ./build/ui/app/video-hotspot-app &
APP_PID=$!
sleep 4   # give Qt Quick time to render the QML
```

> **Platform flags:**
> - `QT_QPA_PLATFORM=xcb` — forces the xcb (X11) platform plugin, required when
>   running under Xvfb. Without this Qt may try eglfs or another backend and fail
>   to create a window.
> - `LD_LIBRARY_PATH=./build/ui/plugin` — ensures the app can locate
>   `libvideo_hotspot_plugin.so` at runtime.
>
> If the window appears blank, verify that `BUILD_UI_PLUGIN=ON` was set at
> CMake configure time — the plugin embeds the QML as a Qt resource. Without
> `BUILD_UI_PLUGIN`, `createWidget()` returns `nullptr` and the app exits with
> code 1.

### 5. Record with ffmpeg (x11grab)

```bash
ffmpeg -y -f x11grab -i :99.0 -video_size 1280x800 -framerate 25 \
  -t 20 \
  -c:v libx264 -preset fast -crf 22 \
  demo/gui-demo.mp4
```

Flags:
| Flag | Meaning |
|------|---------|
| `-f x11grab` | Capture from an X11 display |
| `-i :99.0` | Display `:99`, screen `0` |
| `-video_size 1280x800` | Must match the Xvfb screen geometry |
| `-framerate 25` | Capture at 25 fps |
| `-t 20` | Duration in seconds |
| `-c:v libx264 -crf 22` | H.264 encode, visually lossless |

### 6. Stop the app and Xvfb

```bash
kill $APP_PID
kill $(cat /tmp/.X99-lock)
```

---

## Demo Script

`record-cli.sh` drives the CLI demo session. It wipes the local DB before
running so every recording starts from a clean state. The commands it runs
match the **Quick Start → CLI Usage** section in the main README exactly.

```bash
bash demo/record-cli.sh   # dry run (no recording)
```
