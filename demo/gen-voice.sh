#!/usr/bin/env bash
# gen-voice.sh — Generate narration audio from voiceover.txt
# Requires: espeak-ng, ffmpeg
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
VOICE_DIR="$SCRIPT_DIR/voice"
mkdir -p "$VOICE_DIR"

# ── Lines to synthesise ───────────────────────────────────────────────────────
# Each entry: "line_NN|text"  (must match the timestamp order in voiceover.txt)
declare -a LINES=(
    "line-01|Welcome to Video Hotspot — a decentralised video sharing app built on the Logos stack."
    "line-02|The Map tab shows geotagged video pins published to the Logos network."
    "line-03|Each coloured pin marks a video with GPS coordinates — five clips visible here."
    "line-04|Use the timeline slider to filter videos by time period."
    "line-05|Dragging left narrows the window — older pins fade out."
    "line-06|Dragging right brings all pins back into view."
    "line-07|Pins respond with a smooth opacity transition as the time window changes."
    "line-08|Now let's switch to the Upload tab to see the decentralised upload flow."
    "line-09|Click Upload File to queue a video for publishing to the Logos network."
    "line-10|Each file gets a progress bar as it's chunked and distributed across nodes."
    "line-11|Let's add a second file."
    "line-12|And a third — the queue fills up with live progress tracking."
    "line-13|Click Upload Folder to batch-add an entire directory in one go."
    "line-14|Three folder clips are added simultaneously — the queue grows automatically."
    "line-15|Now watch what happens when we try to upload a file that's already in the queue."
    "line-16|The deduplication engine detects the match and flags it with a DUPLICATE badge — no wasted bandwidth."
    "line-17|Switching to the Downloads tab shows your local cache and owned videos."
    "line-18|Storage usage is tracked against your configured limit — here twelve percent used."
    "line-19|The Settings tab lets you tune storage limits, watched folders, and node connectivity."
    "line-20|Auto-monitoring is enabled — new files dropped into the watched folder are queued instantly."
    "line-21|The Logos node shows connected status — in production this links to a real network peer."
    "line-22|Back on the Map — Video Hotspot gives communities a censorship-resistant way to share video evidence."
    "line-23|Built on Waku for messaging, Codex for storage, and Nomos for consensus — fully on the Logos stack."
    "line-24|Thank you for watching."
)

echo "=== Generating voice clips ==="
for entry in "${LINES[@]}"; do
    name="${entry%%|*}"
    text="${entry#*|}"
    outfile="$VOICE_DIR/${name}.wav"
    echo "  $name: $text"
    espeak-ng -s 135 -p 50 -a 180 -v en-us+m3 "$text" -w "$outfile"
done

# ── Build concat list with silence gaps ──────────────────────────────────────
# Silence padding between clips (in seconds) to match demo timing
declare -a GAPS=(
    "0.0"   # before line-01 (starts at 0:00)
    "0.5"   # gap after line-01
    "0.5"   # gap after line-02
    "0.5"   # gap after line-03
    "0.5"   # gap after line-04
    "0.5"   # gap after line-05
    "0.5"   # gap after line-06
    "0.8"   # gap after line-07 (pause before switching tabs)
    "0.5"   # gap after line-08
    "0.8"   # gap after line-09
    "0.5"   # gap after line-10
    "0.5"   # gap after line-11
    "0.8"   # gap after line-12
    "0.8"   # gap after line-13
    "0.8"   # gap after line-14
    "0.5"   # gap after line-15
    "0.8"   # gap after line-16 (duplicate scene ends)
    "0.5"   # gap after line-17
    "0.8"   # gap after line-18 (switch to settings)
    "0.5"   # gap after line-19
    "0.5"   # gap after line-20
    "0.8"   # gap after line-21 (back to map)
    "0.5"   # gap after line-22
    "0.5"   # gap after line-23
)

echo ""
echo "=== Building concat list ==="

# Generate a 0.5s silence WAV for use as padding
SILENCE_WAV="$VOICE_DIR/silence-half.wav"
ffmpeg -y -f lavfi -i anullsrc=r=22050:cl=mono -t 0.5 "$SILENCE_WAV" -loglevel error

# Generate a 0.8s silence WAV
SILENCE_LONG="$VOICE_DIR/silence-long.wav"
ffmpeg -y -f lavfi -i anullsrc=r=22050:cl=mono -t 0.8 "$SILENCE_LONG" -loglevel error

# Build the input list for concat demuxer
CONCAT_LIST="$VOICE_DIR/concat.txt"
> "$CONCAT_LIST"

for i in "${!LINES[@]}"; do
    num=$((i + 1))
    name=$(printf "line-%02d" $num)
    echo "file '${name}.wav'" >> "$CONCAT_LIST"
    gap="${GAPS[$i]:-0.5}"
    if (( $(echo "$gap > 0.7" | bc -l) )); then
        echo "file 'silence-long.wav'" >> "$CONCAT_LIST"
    elif (( $(echo "$gap > 0.0" | bc -l) )); then
        echo "file 'silence-half.wav'" >> "$CONCAT_LIST"
    fi
done

echo "=== Merging clips → narration.wav ==="
cd "$VOICE_DIR"
ffmpeg -y -f concat -safe 0 -i concat.txt \
    -ar 44100 -ac 1 -acodec pcm_s16le \
    narration.wav -loglevel error

echo ""
echo "=== Narration generated ==="
ls -lh "$VOICE_DIR/narration.wav"
duration=$(ffprobe -v quiet -show_entries format=duration -of csv=p=0 "$VOICE_DIR/narration.wav" 2>/dev/null)
echo "Duration: ${duration}s (~$(echo "scale=0; $duration/60" | bc)m $(echo "scale=0; $duration%60" | bc)s)"
