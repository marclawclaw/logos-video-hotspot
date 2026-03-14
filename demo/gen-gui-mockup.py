#!/usr/bin/env python3
"""
Generate a static SVG mockup of the Video Hotspot Qt GUI.
Based on VideoHotspotApp.qml — dark theme, 4 tabs.
Output: demo/gui-mockup.svg
"""

W, H = 900, 620
TAB_H = 44
CONTENT_H = H - TAB_H

DARK   = "#1a1a1a"
DARK2  = "#2a2a2a"
DARK3  = "#3a3a3a"
TEXT   = "#f0f0f0"
MUTED  = "#888888"
ACCENT = "#4a90d9"
GREEN  = "#4caf50"
ORANGE = "#ff9800"
RED    = "#f44336"
BORDER = "#444444"

def esc(s):
    return s.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")

def rect(x, y, w, h, fill, rx=0, stroke=None, stroke_w=1):
    s = f'stroke="{stroke}" stroke-width="{stroke_w}"' if stroke else 'stroke="none"'
    return f'<rect x="{x}" y="{y}" width="{w}" height="{h}" fill="{fill}" rx="{rx}" {s}/>'

def text(x, y, content, fill=TEXT, size=13, anchor="start", weight="normal", family="monospace"):
    return f'<text x="{x}" y="{y}" fill="{fill}" font-size="{size}" text-anchor="{anchor}" font-weight="{weight}" font-family="{family}">{esc(content)}</text>'

def line(x1, y1, x2, y2, stroke=BORDER, w=1):
    return f'<line x1="{x1}" y1="{y1}" x2="{x2}" y2="{y2}" stroke="{stroke}" stroke-width="{w}"/>'

def progress_bar(x, y, w, h, pct, bg=DARK3, fg=ACCENT, rx=3):
    filled = int(w * pct / 100)
    return rect(x, y, w, h, bg, rx) + rect(x, y, filled, h, fg, rx)


def tab_bar(active=0):
    tabs = ["Upload", "Map", "Downloads", "Settings"]
    out = [rect(0, 0, W, TAB_H, DARK3)]
    tab_w = W // len(tabs)
    for i, label in enumerate(tabs):
        x = i * tab_w
        bg = ACCENT if i == active else DARK3
        out.append(rect(x, 0, tab_w, TAB_H, bg))
        out.append(line(x, 0, x, TAB_H))
        tc = TEXT if i == active else MUTED
        fw = "bold" if i == active else "normal"
        out.append(text(x + tab_w//2, TAB_H//2 + 5, label, fill=tc, size=13, anchor="middle", weight=fw, family="sans-serif"))
    return "\n".join(out)


def upload_screen():
    out = []
    y = TAB_H + 20
    # Section heading
    out.append(text(24, y, "Upload Videos", fill=TEXT, size=16, weight="bold", family="sans-serif"))
    y += 28

    # Buttons row
    btn_w, btn_h = 160, 34
    out.append(rect(24, y, btn_w, btn_h, ACCENT, rx=4))
    out.append(text(24 + btn_w//2, y + btn_h//2 + 5, "Choose File", fill=TEXT, size=12, anchor="middle", family="sans-serif"))
    out.append(rect(200, y, btn_w, btn_h, DARK3, rx=4, stroke=ACCENT))
    out.append(text(200 + btn_w//2, y + btn_h//2 + 5, "Choose Folder", fill=ACCENT, size=12, anchor="middle", family="sans-serif"))

    # Folder monitor toggle
    out.append(text(400, y + btn_h//2 + 5, "Folder monitor:", fill=MUTED, size=12, family="sans-serif"))
    out.append(rect(510, y + 7, 40, 20, GREEN, rx=10))
    out.append(f'<circle cx="540" cy="{y + 17}" r="8" fill="{TEXT}"/>')
    out.append(text(558, y + btn_h//2 + 5, "ON", fill=GREEN, size=11, family="sans-serif", weight="bold"))

    y += btn_h + 20

    # Upload queue header
    out.append(line(24, y, W - 24, y, BORDER))
    y += 12
    out.append(text(24, y, "Upload Queue", fill=MUTED, size=11, family="sans-serif"))
    y += 16

    # Queue items
    items = [
        ("sample1.mp4",     "1.7 KB",  100, "complete",   GREEN),
        ("sample1.mp4",     "1.7 KB",  100, "duplicate",  ORANGE),
        ("folder-demo/clip-a.mp4", "1.7 KB", 100, "complete", GREEN),
        ("folder-demo/clip-b.mp4", "1.7 KB", 100, "complete", GREEN),
    ]
    for fname, size, pct, status, sc in items:
        out.append(rect(24, y, W - 48, 38, DARK2, rx=4))
        # thumbnail placeholder
        out.append(rect(30, y + 5, 44, 28, DARK3, rx=2))
        out.append(text(52, y + 24, "▶", fill=MUTED, size=10, anchor="middle", family="sans-serif"))
        # filename
        out.append(text(84, y + 14, fname, fill=TEXT, size=12, family="monospace"))
        out.append(text(84, y + 28, size, fill=MUTED, size=10, family="monospace"))
        # progress bar
        out.append(progress_bar(240, y + 18, 380, 6, pct, fg=sc))
        # status badge
        out.append(rect(W - 148, y + 10, 86, 18, sc, rx=9))
        out.append(text(W - 148 + 43, y + 23, status, fill=TEXT, size=10, anchor="middle", family="sans-serif", weight="bold"))
        y += 46

    return "\n".join(out)


def map_screen():
    out = []
    cy = TAB_H  # content starts at tab_h

    # Map background (simulated tiles)
    out.append(rect(0, cy, W, CONTENT_H, "#1e2a1e"))

    # Grid lines (tile seams)
    tile = 80
    for gx in range(0, W, tile):
        out.append(line(gx, cy, gx, cy + CONTENT_H, "#263326", 1))
    for gy in range(cy, cy + CONTENT_H, tile):
        out.append(line(0, gy, W, gy, "#263326", 1))

    # Road lines
    out.append(f'<line x1="0" y1="{cy+200}" x2="{W}" y2="{cy+200}" stroke="#2d3d2d" stroke-width="8"/>')
    out.append(f'<line x1="320" y1="{cy}" x2="320" y2="{cy+CONTENT_H}" stroke="#2d3d2d" stroke-width="12"/>')
    out.append(f'<line x1="600" y1="{cy}" x2="600" y2="{cy+CONTENT_H}" stroke="#2d3d2d" stroke-width="6"/>')
    out.append(f'<line x1="0" y1="{cy+380}" x2="{W}" y2="{cy+380}" stroke="#2d3d2d" stroke-width="6"/>')

    # Video pins
    pins = [
        (280, cy+180, ACCENT, "clip-a.mp4", "2026-03-14 09:12"),
        (360, cy+240, ORANGE, "clip-b.mp4", "2026-03-14 10:33"),
        (520, cy+160, ACCENT, "sample1.mp4","2026-03-14 11:07"),
        (440, cy+320, GREEN,  "archive.mp4","2026-03-13 22:45"),
        (650, cy+290, ACCENT, "event-2.mp4","2026-03-14 08:00"),
    ]
    for px, py, pc, pname, ptime in pins:
        out.append(f'<circle cx="{px}" cy="{py}" r="14" fill="{pc}" opacity="0.85"/>')
        out.append(f'<circle cx="{px}" cy="{py}" r="14" fill="none" stroke="{TEXT}" stroke-width="1.5" opacity="0.5"/>')
        out.append(text(px, py + 5, "▶", fill=TEXT, size=10, anchor="middle", family="sans-serif"))

    # Expanded pin popup (sample1.mp4)
    px, py = 520, cy + 160
    bx, by, bw, bh = px - 40, py - 110, 200, 90
    out.append(rect(bx, by, bw, bh, DARK2, rx=6, stroke=ACCENT, stroke_w=1))
    out.append(rect(bx + 8, by + 8, 60, 40, DARK3, rx=3))
    out.append(text(bx + 38, by + 33, "▶", fill=MUTED, size=14, anchor="middle", family="sans-serif"))
    out.append(text(bx + 76, by + 22, "sample1.mp4", fill=TEXT, size=11, family="monospace", weight="bold"))
    out.append(text(bx + 76, by + 36, "2026-03-14 11:07", fill=MUTED, size=10, family="monospace"))
    out.append(text(bx + 76, by + 50, "1.7 KB · user-owned", fill=MUTED, size=10, family="monospace"))
    out.append(rect(bx + 76, by + 60, 70, 20, ACCENT, rx=3))
    out.append(text(bx + 111, by + 74, "Download", fill=TEXT, size=10, anchor="middle", family="sans-serif"))
    # connector line
    out.append(line(px, by + bh, px, py - 14, ACCENT, 1))

    # Timeline slider (bottom)
    slider_y = cy + CONTENT_H - 52
    out.append(rect(0, slider_y, W, 52, DARK2))
    out.append(line(0, slider_y, W, slider_y, BORDER))

    # Granularity selector
    grans = ["hour", "day", "week", "month", "year"]
    gx = 16
    for g in grans:
        active = g == "day"
        bg = ACCENT if active else DARK3
        out.append(rect(gx, slider_y + 8, 50, 18, bg, rx=9))
        out.append(text(gx + 25, slider_y + 21, g, fill=TEXT if active else MUTED,
                        size=10, anchor="middle", family="sans-serif",
                        weight="bold" if active else "normal"))
        gx += 58

    # Slider track + handle
    track_x, track_y = 310, slider_y + 14
    track_w = W - 350
    out.append(rect(track_x, track_y, track_w, 6, DARK3, rx=3))
    out.append(rect(track_x, track_y, int(track_w * 0.65), 6, ACCENT, rx=3))
    handle_x = track_x + int(track_w * 0.65)
    out.append(f'<circle cx="{handle_x}" cy="{track_y+3}" r="9" fill="{ACCENT}"/>')
    out.append(text(handle_x, slider_y + 42, "2026-03-14", fill=MUTED, size=10,
                    anchor="middle", family="monospace"))

    # Zoom controls
    out.append(rect(W - 36, cy + 20, 28, 28, DARK2, rx=4, stroke=BORDER))
    out.append(text(W - 22, cy + 39, "+", fill=TEXT, size=16, anchor="middle", family="sans-serif"))
    out.append(rect(W - 36, cy + 54, 28, 28, DARK2, rx=4, stroke=BORDER))
    out.append(text(W - 22, cy + 73, "−", fill=TEXT, size=16, anchor="middle", family="sans-serif"))

    return "\n".join(out)


def tab_icon(active_tab=0):
    """Compose a full-screen SVG for one tab."""
    parts = [
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{W}" height="{H}" '
        f'style="background:{DARK}; font-family:sans-serif;">',
        # Window chrome
        rect(0, 0, W, H, DARK),
        # Title bar
        rect(0, 0, W, 28, "#111111"),
        f'<circle cx="16" cy="14" r="6" fill="#ff5f57"/>',
        f'<circle cx="34" cy="14" r="6" fill="#febc2e"/>',
        f'<circle cx="52" cy="14" r="6" fill="#28c840"/>',
        text(W//2, 19, "Video Hotspot", fill=MUTED, size=11, anchor="middle", family="sans-serif"),
        # Shift everything down by 28 (title bar)
        f'<g transform="translate(0,28)">',
        tab_bar(active_tab),
    ]

    if active_tab == 0:
        parts.append(upload_screen())
    else:
        parts.append(map_screen())

    parts.append("</g></svg>")
    return "\n".join(parts)


import os
out_dir = os.path.join(os.path.dirname(__file__))

# Primary screenshot: Upload tab
svg_upload = tab_icon(active_tab=0)
with open(os.path.join(out_dir, "gui-upload.svg"), "w") as f:
    f.write(svg_upload)

# Secondary screenshot: Map tab
svg_map = tab_icon(active_tab=1)
with open(os.path.join(out_dir, "gui-map.svg"), "w") as f:
    f.write(svg_map)

print("Generated demo/gui-upload.svg and demo/gui-map.svg")
