#!/usr/bin/env python3
"""
Generate SVG frames showing Video Hotspot embedded inside a Logos Basecamp shell.

Outputs 5 SVG frames + uses rsvg-convert to make PNGs + ffmpeg to make demo-module.mp4.
Each frame shows the Basecamp outer window with sidebar, the Video Hotspot plugin
rendering inside it, and a status footer.

Usage:
    python3 demo/logos-shell-mock/run_demo.py
"""

import os, subprocess, sys
from datetime import date, timedelta

W, H = 1280, 800
SIDEBAR_W = 200
TITLE_H = 36
FOOTER_H = 28
CONTENT_X = SIDEBAR_W
CONTENT_W = W - SIDEBAR_W
CONTENT_Y = TITLE_H
CONTENT_H = H - TITLE_H - FOOTER_H

# Colours
BG       = "#0d1117"
SIDEBAR  = "#161b22"
TITLE_BG = "#0d1117"
FOOTER_BG= "#161b22"
ACCENT   = "#4a90d9"
GREEN    = "#4caf50"
ORANGE   = "#ff9800"
RED      = "#f44336"
TEXT     = "#f0f0f0"
MUTED    = "#8b949e"
DARK     = "#1a1a1a"
DARK2    = "#21262d"
DARK3    = "#30363d"
BORDER   = "#30363d"
LOGOS_BLUE = "#3b82f6"

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
DEMO_DIR = os.path.dirname(SCRIPT_DIR)
OUT_DIR = DEMO_DIR

# ── SVG helpers ──────────────────────────────────────────────────────────────

def esc(s):
    return s.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")

def rect(x, y, w, h, fill, rx=0, stroke=None, stroke_w=1, opacity=1.0):
    s = f'stroke="{stroke}" stroke-width="{stroke_w}"' if stroke else 'stroke="none"'
    op = f' opacity="{opacity}"' if opacity < 1.0 else ""
    return f'<rect x="{x}" y="{y}" width="{w}" height="{h}" fill="{fill}" rx="{rx}" {s}{op}/>'

def text(x, y, content, fill=TEXT, size=13, anchor="start", weight="normal", family="sans-serif"):
    return (f'<text x="{x}" y="{y}" fill="{fill}" font-size="{size}" '
            f'text-anchor="{anchor}" font-weight="{weight}" '
            f'font-family="{family}">{esc(content)}</text>')

def line(x1, y1, x2, y2, stroke=BORDER, w=1):
    return f'<line x1="{x1}" y1="{y1}" x2="{x2}" y2="{y2}" stroke="{stroke}" stroke-width="{w}"/>'

def circle(cx, cy, r, fill, stroke=None, stroke_w=1, opacity=1.0):
    s = f' stroke="{stroke}" stroke-width="{stroke_w}"' if stroke else ""
    op = f' opacity="{opacity}"' if opacity < 1.0 else ""
    return f'<circle cx="{cx}" cy="{cy}" r="{r}" fill="{fill}"{s}{op}/>'

def progress_bar(x, y, w, h, pct, bg=DARK3, fg=ACCENT, rx=3):
    filled = int(w * pct / 100)
    return rect(x, y, w, h, bg, rx) + rect(x, y, filled, h, fg, rx)

def path(d, fill="none", stroke=None, stroke_w=1, linecap="round", dash=""):
    s = f'stroke="{stroke}" stroke-width="{stroke_w}" stroke-linecap="{linecap}"'
    s += f' stroke-dasharray="{dash}"' if dash else ""
    return f'<path d="{d}" fill="{fill}" {s}/>'


# ── Basecamp chrome ──────────────────────────────────────────────────────────

def basecamp_title_bar():
    """Logos Basecamp window title bar."""
    out = [rect(0, 0, W, TITLE_H, TITLE_BG)]
    # Traffic lights
    out.append(circle(16, TITLE_H//2, 6, "#ff5f57"))
    out.append(circle(34, TITLE_H//2, 6, "#febc2e"))
    out.append(circle(52, TITLE_H//2, 6, "#28c840"))
    # Title
    out.append(text(W//2, TITLE_H//2 + 5, "Logos Basecamp", fill=MUTED,
                    size=13, anchor="middle", weight="bold"))
    # Bottom border
    out.append(line(0, TITLE_H, W, TITLE_H, BORDER))
    return "\n".join(out)


def basecamp_sidebar(active_plugin="Video Hotspot"):
    """Left sidebar with plugin list."""
    out = [rect(0, TITLE_H, SIDEBAR_W, H - TITLE_H - FOOTER_H, SIDEBAR)]

    y = TITLE_H + 16
    out.append(text(16, y + 12, "MODULES", fill=MUTED, size=10, weight="bold",
                    family="monospace"))
    y += 28

    plugins = [
        ("Video Hotspot", True),
        ("Chat", False),
        ("Wallet", False),
        ("Browser", False),
        ("Settings", False),
    ]
    for label, is_active in plugins:
        is_selected = (label == active_plugin)
        bg = LOGOS_BLUE if is_selected else "none"
        tc = TEXT if is_selected else MUTED
        fw = "bold" if is_selected else "normal"
        if is_selected:
            out.append(rect(4, y - 4, SIDEBAR_W - 8, 28, bg, rx=4, opacity=0.2))
            out.append(rect(0, y - 4, 3, 28, LOGOS_BLUE, rx=1))
        # Active indicator dot
        if is_active:
            out.append(circle(24, y + 10, 4, GREEN))
        else:
            out.append(circle(24, y + 10, 4, DARK3))
        out.append(text(36, y + 14, label, fill=tc, size=12, weight=fw))
        y += 32

    # Separator
    y += 8
    out.append(line(16, y, SIDEBAR_W - 16, y, BORDER))
    y += 16
    out.append(text(16, y + 12, "NETWORK", fill=MUTED, size=10, weight="bold",
                    family="monospace"))
    y += 28
    out.append(circle(24, y + 4, 4, GREEN))
    out.append(text(36, y + 8, "Waku: 3 peers", fill=MUTED, size=11))
    y += 22
    out.append(circle(24, y + 4, 4, GREEN))
    out.append(text(36, y + 8, "Codex: ready", fill=MUTED, size=11))
    y += 22
    out.append(circle(24, y + 4, 4, ORANGE))
    out.append(text(36, y + 8, "Nomos: sync", fill=MUTED, size=11))

    # Right border
    out.append(line(SIDEBAR_W, TITLE_H, SIDEBAR_W, H - FOOTER_H, BORDER))
    return "\n".join(out)


def basecamp_footer():
    """Status footer showing plugin loaded state."""
    out = [rect(0, H - FOOTER_H, W, FOOTER_H, FOOTER_BG)]
    out.append(line(0, H - FOOTER_H, W, H - FOOTER_H, BORDER))
    out.append(circle(16, H - FOOTER_H//2, 4, GREEN))
    out.append(text(26, H - FOOTER_H//2 + 4,
                    "Plugin: video_hotspot v0.1.0 loaded  |  IComponent: com.logos.component.IComponent",
                    fill=MUTED, size=10, family="monospace"))
    out.append(text(W - 16, H - FOOTER_H//2 + 4, "libvideo_hotspot_plugin.so",
                    fill=MUTED, size=10, anchor="end", family="monospace"))
    return "\n".join(out)


# ── Inner tab bar (inside content area) ──────────────────────────────────────

def inner_tab_bar(active=0):
    """Tab bar within the Video Hotspot plugin content area."""
    tabs = ["Map", "Upload", "Downloads", "Settings"]
    x0 = CONTENT_X
    y0 = CONTENT_Y
    tab_h = 36
    tab_w = CONTENT_W // len(tabs)
    out = [rect(x0, y0, CONTENT_W, tab_h, "#161b22")]
    for i, label in enumerate(tabs):
        x = x0 + i * tab_w
        if i == active:
            out.append(rect(x, y0, tab_w, tab_h, DARK2))
            out.append(rect(x, y0 + tab_h - 3, tab_w, 3, LOGOS_BLUE))
        out.append(line(x, y0, x, y0 + tab_h, BORDER))
        tc = TEXT if i == active else MUTED
        fw = "bold" if i == active else "normal"
        out.append(text(x + tab_w//2, y0 + tab_h//2 + 5, label,
                        fill=tc, size=12, anchor="middle", weight=fw))
    out.append(line(x0, y0 + tab_h, W, y0 + tab_h, BORDER))
    return "\n".join(out), y0 + tab_h


# ── Screen content (inside Basecamp frame) ───────────────────────────────────

def map_content(slider_pct=0.60):
    """Map screen rendered inside Basecamp content area."""
    out = []
    tab_bar_svg, y0 = inner_tab_bar(active=0)
    out.append(tab_bar_svg)

    map_bottom = H - FOOTER_H - 50
    mh = map_bottom - y0

    # Terrain
    out.append(rect(CONTENT_X, y0, CONTENT_W, mh, "#1c2b1c"))

    # Grid
    tile = 80
    grid = ""
    for gx in range(CONTENT_X, W, tile):
        grid += f"M{gx},{y0} L{gx},{y0+mh} "
    for gy in range(y0, y0+mh, tile):
        grid += f"M{CONTENT_X},{gy} L{W},{gy} "
    out.append(path(grid, stroke="#243224", stroke_w=0.5))

    # Roads
    cx = CONTENT_X + CONTENT_W//2
    out.append(line(cx, y0, cx, y0+mh, "#2e3e2e", 12))
    out.append(line(CONTENT_X, y0 + mh//3, W, y0 + mh//3, "#2e3e2e", 8))
    out.append(line(CONTENT_X, y0 + 2*mh//3, W, y0 + 2*mh//3, "#283828", 5))

    # Video pins
    pins = [
        (0.15, 0.25, 0.20, ACCENT, "clip-a.mp4"),
        (0.35, 0.45, 0.38, ORANGE, "clip-b.mp4"),
        (0.55, 0.22, 0.55, ACCENT, "sample1.mp4"),
        (0.72, 0.55, 0.72, GREEN,  "archive.mp4"),
        (0.88, 0.35, 0.88, ACCENT, "event-2.mp4"),
    ]
    visible = 0
    for fx, fy, tv, pc, lbl in pins:
        px = int(CONTENT_X + fx * CONTENT_W)
        py = int(y0 + fy * mh)
        op = 1.0 if tv <= slider_pct else 0.15
        if op > 0.5:
            visible += 1
        out.append(circle(px, py, 12, pc, opacity=op))
        out.append(circle(px, py, 12, "none", stroke=TEXT, stroke_w=1.5, opacity=op*0.5))
        out.append(text(px, py + 4, "V", fill=TEXT, size=8, anchor="middle", weight="bold"))

    # Popup on sample1
    if 0.55 <= slider_pct:
        px = int(CONTENT_X + 0.55 * CONTENT_W)
        py = int(y0 + 0.22 * mh)
        bx, by, bw, bh = px - 30, py - 85, 190, 75
        out.append(rect(bx, by, bw, bh, DARK2, rx=5, stroke=ACCENT, stroke_w=1))
        out.append(text(bx+10, by+18, "sample1.mp4", fill=TEXT, size=11, weight="bold", family="monospace"))
        out.append(text(bx+10, by+32, "2026-03-14 11:07", fill=MUTED, size=10, family="monospace"))
        out.append(text(bx+10, by+46, "1.7 KB  user-owned", fill=MUTED, size=10, family="monospace"))
        out.append(rect(bx+10, by+54, 60, 16, ACCENT, rx=3))
        out.append(text(bx+40, by+66, "Download", fill=TEXT, size=9, anchor="middle"))
        out.append(line(px, by+bh, px, py-12, ACCENT))

    # Badge
    out.append(rect(CONTENT_X+10, y0+10, 145, 22, DARK2, rx=4, stroke=BORDER))
    out.append(text(CONTENT_X+18, y0+25, f"{visible} of {len(pins)} videos",
                    fill=ACCENT, size=11, family="monospace"))

    # Zoom
    out.append(rect(W-40, y0+10, 26, 26, DARK2, rx=4, stroke=BORDER))
    out.append(text(W-27, y0+28, "+", fill=TEXT, size=14, anchor="middle", weight="bold"))
    out.append(rect(W-40, y0+42, 26, 26, DARK2, rx=4, stroke=BORDER))
    out.append(text(W-27, y0+60, "-", fill=TEXT, size=14, anchor="middle", weight="bold"))

    # Timeline strip
    sy = y0 + mh
    out.append(rect(CONTENT_X, sy, CONTENT_W, 50, "#111111"))
    out.append(line(CONTENT_X, sy, W, sy, BORDER))
    out.append(text(CONTENT_X+12, sy+30, "Timeline:", fill=MUTED, size=11))

    tx, tw = CONTENT_X + 90, CONTENT_W - 130
    ty = sy + 24
    out.append(rect(tx, ty-3, tw, 6, DARK3, rx=3))
    filled_w = int(tw * slider_pct)
    out.append(rect(tx, ty-3, filled_w, 6, ACCENT, rx=3))
    hx = tx + filled_w
    out.append(circle(hx, ty, 8, ACCENT, stroke=TEXT, stroke_w=1))

    base = date(2026, 3, 14)
    offset_days = round((1.0 - slider_pct) * 30)
    d = base - timedelta(days=offset_days)
    h_val = round(slider_pct * 23)
    date_lbl = f"{d.strftime('%Y-%m-%d')} {h_val:02d}:00"
    out.append(text(hx, sy+44, date_lbl, fill=MUTED, size=9, anchor="middle", family="monospace"))

    # Granularity buttons
    gx = CONTENT_X + 90
    for glabel in ["Hour", "Day", "Week", "Month", "Year"]:
        is_sel = (glabel == "Day")
        gc = LOGOS_BLUE if is_sel else DARK3
        out.append(rect(gx, sy+4, 40, 16, gc, rx=3))
        out.append(text(gx+20, sy+15, glabel, fill=TEXT if is_sel else MUTED, size=9, anchor="middle"))
        gx += 46

    return "\n".join(out)


def upload_content():
    """Upload screen inside Basecamp."""
    out = []
    tab_bar_svg, y0 = inner_tab_bar(active=1)
    out.append(tab_bar_svg)

    out.append(rect(CONTENT_X, y0, CONTENT_W, CONTENT_H, BG))
    y = y0 + 20
    out.append(text(CONTENT_X+20, y, "Upload Videos", fill=TEXT, size=16, weight="bold"))
    y += 28

    btn_w, btn_h = 140, 32
    for bx, label, bg, tc in [
        (CONTENT_X+20, "Upload File", DARK3, TEXT),
        (CONTENT_X+172, "Upload Folder", DARK3, TEXT),
        (CONTENT_X+324, "Monitor Folder", "#1a3a1a", "#aaffaa"),
    ]:
        out.append(rect(bx, y, btn_w, btn_h, bg, rx=4))
        out.append(text(bx+btn_w//2, y+btn_h//2+5, label, fill=tc, size=11, anchor="middle"))

    y += btn_h + 16
    out.append(line(CONTENT_X+20, y, W-20, y, BORDER))
    y += 14
    out.append(text(CONTENT_X+20, y, "Upload Queue", fill=MUTED, size=12, weight="bold"))
    out.append(rect(CONTENT_X+120, y-12, 20, 16, ACCENT, rx=8))
    out.append(text(CONTENT_X+130, y-1, "5", fill=TEXT, size=10, anchor="middle", weight="bold"))
    y += 16

    items = [
        ("event-footage-001.mp4",  100, "done",      GREEN,  "DONE"),
        ("protest-march-clip2.mp4",100, "done",      GREEN,  "DONE"),
        ("folder-clip-001.mp4",    100, "done",      GREEN,  "DONE"),
        ("folder-clip-002.mp4",    100, "done",      GREEN,  "DONE"),
        ("event-footage-001.mp4",  100, "duplicate", ORANGE, "DUPLICATE"),
    ]
    item_h = 46
    for fname, pct, status, sc, badge in items:
        out.append(rect(CONTENT_X+20, y, CONTENT_W-40, item_h-4, DARK2, rx=4))
        icon_char = "!" if status == "duplicate" else "C"
        icon_color = ORANGE if status == "duplicate" else GREEN
        out.append(circle(CONTENT_X+38, y+(item_h-4)//2, 9, icon_color))
        out.append(text(CONTENT_X+38, y+(item_h-4)//2+4, icon_char, fill=TEXT, size=10, anchor="middle", weight="bold"))
        out.append(text(CONTENT_X+56, y+(item_h-4)//2+4, fname, fill=TEXT, size=11, family="monospace"))
        badge_w = len(badge)*7+16
        out.append(rect(W-badge_w-30, y+10, badge_w, 20, sc, rx=10))
        out.append(text(W-badge_w//2-30, y+24, badge, fill=TEXT, size=9, anchor="middle", weight="bold"))
        y += item_h

    return "\n".join(out)


def timeline_content():
    """Map view focused on timeline slider interaction."""
    # Same as map but with slider at a different position to show interaction
    return map_content(slider_pct=0.35)


def downloads_content():
    """Downloads screen inside Basecamp."""
    out = []
    tab_bar_svg, y0 = inner_tab_bar(active=2)
    out.append(tab_bar_svg)

    out.append(rect(CONTENT_X, y0, CONTENT_W, CONTENT_H, BG))
    y = y0 + 20
    out.append(text(CONTENT_X+20, y, "Downloads & Cache", fill=TEXT, size=16, weight="bold"))
    y += 26
    out.append(text(CONTENT_X+20, y, "Storage: 1.2 GB of 10 GB used (12%)", fill=MUTED, size=12))
    y += 16
    bar_w = CONTENT_W - 40
    out.append(rect(CONTENT_X+20, y, bar_w, 14, DARK3, rx=6))
    out.append(rect(CONTENT_X+20, y, int(bar_w*0.12), 14, ACCENT, rx=6))
    y += 28
    out.append(line(CONTENT_X+20, y, W-20, y, BORDER))
    y += 14
    out.append(text(CONTENT_X+20, y, "Your Videos (3)", fill=MUTED, size=12, weight="bold"))
    y += 18

    videos = [
        ("sample-event.mp4", "2.3 MB", "2026-03-14"),
        ("march-footage.mp4", "8.1 MB", "2026-03-13"),
        ("clip-b.mp4", "4.5 MB", "2026-03-12"),
    ]
    for vname, vsize, vdate in videos:
        out.append(rect(CONTENT_X+20, y, CONTENT_W-40, 42, DARK2, rx=4))
        out.append(circle(CONTENT_X+38, y+21, 8, GREEN))
        out.append(text(CONTENT_X+54, y+17, vname, fill=TEXT, size=12, weight="bold", family="monospace"))
        out.append(text(CONTENT_X+54, y+32, f"{vsize}  {vdate}", fill=MUTED, size=10, family="monospace"))
        out.append(rect(W-100, y+10, 64, 20, "#1a3a1a", rx=10, stroke=GREEN, stroke_w=1))
        out.append(text(W-68, y+24, "Owned", fill=GREEN, size=9, anchor="middle"))
        y += 48

    return "\n".join(out)


def plugin_loaded_content():
    """Settings-like screen emphasizing the plugin loaded state."""
    out = []
    tab_bar_svg, y0 = inner_tab_bar(active=3)
    out.append(tab_bar_svg)

    out.append(rect(CONTENT_X, y0, CONTENT_W, CONTENT_H, BG))
    y = y0 + 20
    out.append(text(CONTENT_X+20, y, "Settings", fill=TEXT, size=16, weight="bold"))
    y += 32

    rows = [
        ("Storage limit", "10 GB"),
        ("Monitor folder", "~/Videos/Hotspot"),
        ("Auto-monitoring", "Enabled"),
        ("Scan interval", "30 seconds"),
        ("Logos node", "connected (mock)"),
        ("Waku peers", "3 connected"),
    ]
    rh = 44
    for label, value in rows:
        out.append(rect(CONTENT_X+20, y, CONTENT_W-40, rh-4, DARK2, rx=4))
        out.append(text(CONTENT_X+40, y+rh//2+4, label, fill=MUTED, size=12))
        is_status = "connected" in value or "Enabled" in value
        vc = GREEN if is_status else TEXT
        out.append(text(CONTENT_X+220, y+rh//2+4, value, fill=vc, size=12, family="monospace"))
        y += rh

    y += 16
    out.append(text(CONTENT_X+20, y+14,
                    "Video Hotspot v0.1.0  |  Logos stack: Waku, Codex, Nomos",
                    fill=MUTED, size=11))

    return "\n".join(out)


# ── Compose full frame ───────────────────────────────────────────────────────

def compose(content_fn):
    parts = [
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{W}" height="{H}">',
        rect(0, 0, W, H, BG),
        basecamp_title_bar(),
        basecamp_sidebar(),
        content_fn(),
        basecamp_footer(),
        "</svg>",
    ]
    return "\n".join(parts)


# ── Generate frames ──────────────────────────────────────────────────────────

def main():
    frames = [
        ("basecamp-launch",   lambda: plugin_loaded_content()),  # launch/loading
        ("basecamp-map",      lambda: map_content(slider_pct=0.60)),
        ("basecamp-timeline", lambda: timeline_content()),
        ("basecamp-upload",   lambda: upload_content()),
        ("basecamp-plugin",   lambda: plugin_loaded_content()),
    ]

    svg_dir = os.path.join(SCRIPT_DIR, "frames")
    os.makedirs(svg_dir, exist_ok=True)

    svg_paths = []
    for name, fn in frames:
        svg_path = os.path.join(svg_dir, f"{name}.svg")
        with open(svg_path, "w") as f:
            f.write(compose(fn))
        svg_paths.append((name, svg_path))
        print(f"  Generated {name}.svg")

    # Convert to PNGs for snapshots
    snapshot_names = [
        "snapshot_01_launch",
        "snapshot_02_map",
        "snapshot_03_timeline",
        "snapshot_04_upload",
        "snapshot_05_plugin_loaded",
    ]

    png_paths = []
    for (name, svg_path), snap_name in zip(svg_paths, snapshot_names):
        png_path = os.path.join(OUT_DIR, f"{snap_name}.png")
        try:
            subprocess.run([
                "rsvg-convert", "-w", str(W), "-h", str(H), svg_path, "-o", png_path
            ], check=True)
            png_paths.append(png_path)
            print(f"  Converted {snap_name}.png")
        except FileNotFoundError:
            # Fallback: use ImageMagick convert
            subprocess.run([
                "convert", "-size", f"{W}x{H}", f"svg:{svg_path}", png_path
            ], check=True)
            png_paths.append(png_path)
            print(f"  Converted {snap_name}.png (via ImageMagick)")

    # Build demo-module.mp4 from PNGs
    print("\n  Assembling demo-module.mp4...")
    # Each frame shown for 5 seconds = 25 seconds total
    seg_durs = [5, 5, 5, 5, 5]
    tmp_dir = os.path.join(SCRIPT_DIR, ".tmp")
    os.makedirs(tmp_dir, exist_ok=True)

    concat_list = os.path.join(tmp_dir, "concat.txt")
    with open(concat_list, "w") as cl:
        for i, (png_path, dur) in enumerate(zip(png_paths, seg_durs)):
            seg_out = os.path.join(tmp_dir, f"seg-{i}.mp4")
            subprocess.run([
                "ffmpeg", "-y", "-loglevel", "error",
                "-loop", "1", "-i", png_path,
                "-t", str(dur),
                "-vf", f"scale={W}:{H},format=yuv420p",
                "-r", "25",
                "-c:v", "libx264", "-preset", "fast", "-crf", "22",
                seg_out
            ], check=True)
            cl.write(f"file '{seg_out}'\n")

    out_mp4 = os.path.join(OUT_DIR, "demo-module.mp4")
    subprocess.run([
        "ffmpeg", "-y", "-loglevel", "error",
        "-f", "concat", "-safe", "0", "-i", concat_list,
        "-c", "copy",
        out_mp4
    ], check=True)

    # Cleanup
    import shutil
    shutil.rmtree(tmp_dir, ignore_errors=True)

    print(f"\n  Output: {out_mp4}")
    print(f"  Snapshots: {', '.join(os.path.basename(p) for p in png_paths)}")
    print("  Done.")


if __name__ == "__main__":
    main()
