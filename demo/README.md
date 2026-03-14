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

## Demo Script

`record-cli.sh` drives the CLI demo session. It wipes the local DB before
running so every recording starts from a clean state. The commands it runs
match the **Quick Start → CLI Usage** section in the main README exactly.

```bash
bash demo/record-cli.sh   # dry run (no recording)
```
