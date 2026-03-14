#!/usr/bin/env python3
"""
Generate static SVG mockups of the Video Hotspot Qt GUI.
Tab order mirrors VideoHotspotApp.qml: Map → Upload → Downloads → Settings
Outputs: demo/gui-map.svg  demo/gui-upload.svg  demo/gui-downloads.svg  demo/gui-settings.svg
"""

W, H = 1280, 800
TITLE_H = 40
TAB_H   = 44
CHROME_H = TITLE_H + TAB_H   # 84 px
CONTENT_H = H - CHROME_H

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

def progress_bar(x, y, w, h, pct, bg=DARK3, fg=ACCENT, rx=3):
    filled = int(w * pct / 100)
    return rect(x, y, w, h, bg, rx) + rect(x, y, filled, h, fg, rx)

def circle(cx, cy, r, fill, stroke=None, stroke_w=1, opacity=1.0):
    s = f' stroke="{stroke}" stroke-width="{stroke_w}"' if stroke else ""
    op = f' opacity="{opacity}"' if opacity < 1.0 else ""
    return f'<circle cx="{cx}" cy="{cy}" r="{r}" fill="{fill}"{s}{op}/>'

def path(d, fill="none", stroke=None, stroke_w=1, linecap="round", dash=""):
    s = f'stroke="{stroke}" stroke-width="{stroke_w}" stroke-linecap="{linecap}"'
    s += f' stroke-dasharray="{dash}"' if dash else ""
    return f'<path d="{d}" fill="{fill}" {s}/>'


# ── Chrome ────────────────────────────────────────────────────────────────────

def title_bar():
    out = [rect(0, 0, W, TITLE_H, "#111111")]
    out.append(circle(18, TITLE_H//2, 7, "#ff5f57"))
    out.append(circle(38, TITLE_H//2, 7, "#febc2e"))
    out.append(circle(58, TITLE_H//2, 7, "#28c840"))
    out.append(text(W//2, TITLE_H//2 + 5, "Video Hotspot", fill=MUTED,
                    size=12, anchor="middle"))
    return "\n".join(out)


def tab_bar(active=0):
    # Tab order: Map | Upload | Downloads | Settings
    tabs = ["Map", "Upload", "Downloads", "Settings"]
    out  = [rect(0, TITLE_H, W, TAB_H, "#222222")]
    tab_w = W // len(tabs)
    for i, label in enumerate(tabs):
        x  = i * tab_w
        bg = ACCENT if i == active else "#222222"
        out.append(rect(x, TITLE_H, tab_w, TAB_H, bg))
        out.append(line(x, TITLE_H, x, TITLE_H + TAB_H, "#333333"))
        tc = TEXT   if i == active else MUTED
        fw = "bold" if i == active else "normal"
        out.append(text(x + tab_w // 2, TITLE_H + TAB_H // 2 + 5, label,
                        fill=tc, size=13, anchor="middle", weight=fw))
    return "\n".join(out)


# ── Map screen ────────────────────────────────────────────────────────────────

def map_screen(slider_pct=0.60):
    """Draw the Map screen.  slider_pct drives which pins appear active."""
    out = []
    y0  = CHROME_H           # top of map content
    mh  = CONTENT_H - 60     # map area height (leave 60 for timeline strip)

    # ── Terrain base ─────────────────────────────────────────────────────
    out.append(rect(0, y0, W, mh, "#1c2b1c"))

    # Tile grid — thin lines, low contrast
    tile = 80
    grid_lines = ""
    for gx in range(0, W, tile):
        grid_lines += f"M{gx},{y0} L{gx},{y0+mh} "
    for gy in range(y0, y0+mh, tile):
        grid_lines += f"M0,{gy} L{W},{gy} "
    out.append(path(grid_lines, stroke="#243224", stroke_w=0.5))

    # City blocks
    blocks = [
        (0.10, 0.10, 0.18, 0.22),
        (0.42, 0.12, 0.20, 0.18),
        (0.68, 0.08, 0.14, 0.16),
        (0.08, 0.50, 0.16, 0.20),
        (0.44, 0.48, 0.18, 0.22),
        (0.70, 0.58, 0.16, 0.18),
        (0.20, 0.70, 0.22, 0.16),
        (0.56, 0.74, 0.20, 0.14),
    ]
    for bx, by, bw, bh in blocks:
        out.append(rect(int(bx*W), int(y0 + by*mh),
                        int(bw*W), int(bh*mh), "#1e2e1e"))

    # Roads — main arterials
    out.append(line(int(0.36*W), y0, int(0.36*W), y0+mh, "#2e3e2e", 14))
    out.append(line(0, int(y0 + 0.36*mh), W, int(y0 + 0.36*mh), "#2e3e2e", 10))
    # Secondary
    out.append(line(int(0.65*W), y0, int(0.65*W), y0+mh, "#283828", 6))
    out.append(line(0, int(y0 + 0.65*mh), W, int(y0 + 0.65*mh), "#283828", 6))
    # Minor streets
    minor = [0.12, 0.28, 0.50, 0.76, 0.90]
    for mx in minor:
        out.append(line(int(mx*W), y0, int(mx*W), y0+mh, "#243024", 2))
    for my in [0.20, 0.50, 0.80]:
        out.append(line(0, int(y0+my*mh), W, int(y0+my*mh), "#243024", 2))

    # Centre-line dashes on arterials
    dash_path = (f"M{int(0.36*W)},{y0} L{int(0.36*W)},{y0+mh} "
                 f"M0,{int(y0+0.36*mh)} L{W},{int(y0+0.36*mh)}")
    out.append(path(dash_path, stroke="#3a4a1a", stroke_w=1, dash="8 12"))

    # ── Video pins ────────────────────────────────────────────────────────
    # timeValue matches QML ListModel order
    pins = [
        (0.22, 0.30, 0.20, ACCENT, "clip-a.mp4"),
        (0.38, 0.44, 0.38, ORANGE, "clip-b.mp4"),
        (0.55, 0.26, 0.55, ACCENT, "sample1.mp4"),
        (0.70, 0.52, 0.72, GREEN,  "archive.mp4"),
        (0.82, 0.38, 0.88, ACCENT, "event-2.mp4"),
    ]
    visible_count = 0
    for fx, fy, tv, pc, lbl in pins:
        px = int(fx * W)
        py = int(y0 + fy * mh)
        op = 1.0 if tv <= slider_pct else 0.15
        if op > 0.5:
            visible_count += 1
        out.append(circle(px, py, 14, pc,  opacity=op))
        out.append(circle(px, py, 14, "none", stroke=TEXT, stroke_w=1.5, opacity=op*0.5))
        out.append(text(px, py + 5, "▶", fill=TEXT, size=9,
                        anchor="middle", weight="bold"))

    # Expanded popup on sample1.mp4 (if visible)
    fx, fy, tv = 0.55, 0.26, 0.55
    if tv <= slider_pct:
        px = int(fx * W)
        py = int(y0 + fy * mh)
        bx, by, bw, bh = px - 40, py - 100, 210, 90
        out.append(rect(bx, by, bw, bh, DARK2, rx=6, stroke=ACCENT, stroke_w=1))
        out.append(rect(bx+8, by+8, 58, 38, DARK3, rx=3))
        out.append(text(bx+37, by+31, "▶", fill=MUTED, size=14, anchor="middle"))
        out.append(text(bx+74, by+22, "sample1.mp4", fill=TEXT, size=11,
                        weight="bold", family="monospace"))
        out.append(text(bx+74, by+36, "2026-03-14 11:07", fill=MUTED, size=10,
                        family="monospace"))
        out.append(text(bx+74, by+50, "1.7 KB · user-owned", fill=MUTED, size=10,
                        family="monospace"))
        out.append(rect(bx+74, by+60, 70, 20, ACCENT, rx=3))
        out.append(text(bx+109, by+74, "Download", fill=TEXT, size=10, anchor="middle"))
        out.append(line(px, by+bh, px, py-14, ACCENT))

    # Pins-visible counter badge
    out.append(rect(12, y0+12, 160, 24, DARK2, rx=4, stroke="#444444", stroke_w=1))
    out.append(text(20, y0+28,
                    f"{visible_count} of {len(pins)} videos",
                    fill=ACCENT, size=11, family="monospace"))

    # Zoom controls
    out.append(rect(W-44, y0+16, 28, 28, DARK2, rx=4, stroke=BORDER))
    out.append(text(W-30, y0+35, "+", fill=TEXT, size=16, anchor="middle", weight="bold"))
    out.append(rect(W-44, y0+52, 28, 28, DARK2, rx=4, stroke=BORDER))
    out.append(text(W-30, y0+71, "−", fill=TEXT, size=16, anchor="middle", weight="bold"))

    # ── Timeline strip ────────────────────────────────────────────────────
    sy = y0 + mh   # timeline strip top
    out.append(rect(0, sy, W, 60, "#111111"))
    out.append(line(0, sy, W, sy, "#333333"))

    # "Timeline:" label
    out.append(text(14, sy + 36, "Timeline:", fill=MUTED, size=12))

    # Track
    tx, tw = 110, W - 160
    ty = sy + 28
    out.append(rect(tx, ty - 3, tw, 6, DARK3, rx=3))
    filled_w = int(tw * slider_pct)
    out.append(rect(tx, ty - 3, filled_w, 6, ACCENT, rx=3))

    # Handle
    hx = tx + filled_w
    out.append(circle(hx, ty, 9, ACCENT, stroke=TEXT, stroke_w=1))

    # Date label
    from datetime import date, timedelta
    base = date(2026, 3, 14)
    offset_days = round((1.0 - slider_pct) * 30)
    d = base - timedelta(days=offset_days)
    h_val = round(slider_pct * 23)
    date_lbl = f"{d.strftime('%Y-%m-%d')} {h_val:02d}:00"
    out.append(text(hx, sy + 52, date_lbl, fill=MUTED, size=10, anchor="middle",
                    family="monospace"))

    return "\n".join(out)


# ── Upload screen ─────────────────────────────────────────────────────────────

def upload_screen():
    out = []
    y = CHROME_H + 24

    out.append(text(24, y, "Upload Videos", fill=TEXT, size=18, weight="bold"))
    y += 32

    # Buttons row
    btn_w, btn_h = 160, 36
    for bx, label, bg, tc in [
        (24,  "Upload File",    DARK3,  TEXT),
        (200, "Upload Folder",  DARK3,  TEXT),
        (376, "Monitor Folder", "#2a5a2a", "#aaffaa"),
    ]:
        out.append(rect(bx, y, btn_w, btn_h, bg, rx=4))
        out.append(text(bx + btn_w//2, y + btn_h//2 + 5, label,
                        fill=tc, size=12, anchor="middle"))

    y += btn_h + 20
    out.append(line(24, y, W - 24, y, BORDER))
    y += 16
    out.append(text(24, y, "Upload Queue", fill=MUTED, size=13, weight="bold"))

    # Queue count badge
    out.append(rect(140, y - 12, 22, 18, ACCENT, rx=9))
    out.append(text(151, y, "5", fill=TEXT, size=11, anchor="middle", weight="bold"))
    y += 20

    items = [
        ("event-footage-001.mp4",  100, "done",      GREEN,  "DONE"),
        ("protest-march-clip2.mp4",100, "done",      GREEN,  "DONE"),
        ("folder-clip-001.mp4",    100, "done",      GREEN,  "DONE"),
        ("folder-clip-002.mp4",    100, "done",      GREEN,  "DONE"),
        ("event-footage-001.mp4",  100, "duplicate", ORANGE, "DUPLICATE"),
    ]
    item_h = 52
    for fname, pct, status, sc, badge in items:
        out.append(rect(24, y, W - 48, item_h - 6, DARK2, rx=5))
        # icon
        if status == "duplicate":
            icon_color = ORANGE
            icon_char = "!"
        elif status == "done":
            icon_color = GREEN
            icon_char = "✓"
        else:
            icon_color = MUTED
            icon_char = "…"
        out.append(circle(44, y + (item_h-6)//2, 10, icon_color))
        out.append(text(44, y + (item_h-6)//2 + 5, icon_char, fill=TEXT,
                        size=11, anchor="middle", weight="bold"))
        # filename
        out.append(text(62, y + (item_h-6)//2 + 4, fname, fill=TEXT, size=12,
                        family="monospace"))
        # status badge
        badge_w = len(badge) * 7 + 20
        out.append(rect(W - badge_w - 40, y + 10, badge_w, 22, sc, rx=11))
        out.append(text(W - badge_w//2 - 40, y + 25, badge,
                        fill=TEXT, size=10, anchor="middle", weight="bold"))
        y += item_h

    return "\n".join(out)


# ── Downloads screen ─────────────────────────────────────────────────────────

def downloads_screen():
    out = []
    y = CHROME_H + 24

    out.append(text(24, y, "Downloads & Cache", fill=TEXT, size=18, weight="bold"))
    y += 30

    # Storage bar
    out.append(text(24, y, "Storage: 1.2 GB of 10 GB used  (12%)", fill=MUTED, size=13))
    y += 18
    bar_w = W - 48
    out.append(rect(24, y, bar_w, 16, DARK3, rx=6))
    out.append(rect(24, y, int(bar_w * 0.12), 16, ACCENT, rx=6))
    y += 32

    out.append(line(24, y, W-24, y, BORDER))
    y += 16

    out.append(text(24, y, "Your Videos  (3)", fill=MUTED, size=13, weight="bold"))
    y += 20

    videos = [
        ("sample-event.mp4",   "2.3 MB",  "2026-03-14 08:12"),
        ("march-footage.mp4",  "8.1 MB",  "2026-03-13 15:44"),
        ("clip-b.mp4",         "4.5 MB",  "2026-03-12 22:31"),
    ]
    item_h = 52
    for vname, vsize, vdate in videos:
        out.append(rect(24, y, W - 48, item_h - 6, DARK2, rx=5))
        out.append(circle(44, y + (item_h-6)//2, 10, GREEN))
        out.append(text(44, y + (item_h-6)//2 + 5, "✓", fill=TEXT,
                        size=11, anchor="middle", weight="bold"))
        out.append(text(62, y + (item_h-6)//2 - 5, vname, fill=TEXT, size=13,
                        family="monospace", weight="bold"))
        out.append(text(62, y + (item_h-6)//2 + 12, f"{vsize}  ·  {vdate}",
                        fill=MUTED, size=11, family="monospace"))
        # Owned badge
        out.append(rect(W - 120, y + 12, 80, 20, "#1a3a1a", rx=10, stroke=GREEN, stroke_w=1))
        out.append(text(W - 80, y + 26, "✓ Owned", fill=GREEN, size=10, anchor="middle"))
        y += item_h

    y += 16
    out.append(line(24, y, W-24, y, BORDER))
    y += 16

    # Cache stats row
    out.append(text(24, y, "Cached (Peers)", fill=MUTED, size=13, weight="bold"))
    y += 20
    out.append(rect(24, y, W - 48, item_h - 6, DARK2, rx=5))
    out.append(text(62, y + (item_h-6)//2 + 4, "No cached peer content yet — browse the Map to discover videos",
                    fill=MUTED, size=12))
    y += item_h

    return "\n".join(out)


# ── Settings screen ───────────────────────────────────────────────────────────

def settings_screen():
    out = []
    y = CHROME_H + 24

    out.append(text(24, y, "Settings", fill=TEXT, size=18, weight="bold"))
    y += 36

    LABEL_W = 200
    ROW_H = 52

    def row_bg(y, h=ROW_H):
        return rect(24, y, W - 48, h - 6, DARK2, rx=5)

    # ── Storage limit ──────────────────────────────────────────────────────
    out.append(row_bg(y))
    out.append(text(44, y + ROW_H//2 + 4, "Storage limit", fill=MUTED, size=13))
    # Mini slider
    sx, sw = 44 + LABEL_W, W - 48 - LABEL_W - 140
    sy2 = y + ROW_H//2
    slider_pct = 0.20  # 10 of 50 GB
    out.append(rect(sx, sy2 - 3, sw, 6, DARK3, rx=3))
    out.append(rect(sx, sy2 - 3, int(sw * slider_pct), 6, ACCENT, rx=3))
    out.append(circle(sx + int(sw * slider_pct), sy2, 9, ACCENT, stroke=TEXT, stroke_w=1))
    out.append(text(sx + sw + 16, y + ROW_H//2 + 5, "10 GB", fill=ACCENT, size=13,
                    weight="bold"))
    y += ROW_H

    # ── Monitor folder ─────────────────────────────────────────────────────
    out.append(row_bg(y))
    out.append(text(44, y + ROW_H//2 + 4, "Monitor folder", fill=MUTED, size=13))
    out.append(text(44 + LABEL_W, y + ROW_H//2 + 4, "~/Videos/Hotspot", fill=TEXT,
                    size=13, family="monospace"))
    out.append(rect(W - 120, y + 12, 76, 26, DARK3, rx=4))
    out.append(text(W - 82, y + 30, "Change", fill=TEXT, size=12, anchor="middle"))
    y += ROW_H

    # ── Auto-monitoring toggle ─────────────────────────────────────────────
    out.append(row_bg(y))
    out.append(text(44, y + ROW_H//2 + 4, "Auto-monitoring", fill=MUTED, size=13))
    # Toggle switch (on = green)
    tx2, tw2 = 44 + LABEL_W, 48
    th2 = 24
    ty2 = y + ROW_H//2 - th2//2
    out.append(rect(tx2, ty2, tw2, th2, GREEN, rx=th2//2))
    out.append(circle(tx2 + tw2 - th2//2 - 2, ty2 + th2//2, th2//2 - 3, TEXT))
    out.append(text(tx2 + tw2 + 16, y + ROW_H//2 + 5, "Enabled", fill=GREEN, size=12))
    y += ROW_H

    # ── Watched folder interval ────────────────────────────────────────────
    out.append(row_bg(y))
    out.append(text(44, y + ROW_H//2 + 4, "Scan interval", fill=MUTED, size=13))
    out.append(text(44 + LABEL_W, y + ROW_H//2 + 4, "30 seconds", fill=TEXT, size=13))
    y += ROW_H

    # ── Logos node ─────────────────────────────────────────────────────────
    out.append(row_bg(y))
    out.append(text(44, y + ROW_H//2 + 4, "Logos node", fill=MUTED, size=13))
    out.append(circle(44 + LABEL_W + 8, y + ROW_H//2, 6, GREEN))
    out.append(text(44 + LABEL_W + 22, y + ROW_H//2 + 5, "connected  (mock)",
                    fill=GREEN, size=13))
    y += ROW_H

    # ── Network peers ──────────────────────────────────────────────────────
    out.append(row_bg(y))
    out.append(text(44, y + ROW_H//2 + 4, "Waku peers", fill=MUTED, size=13))
    out.append(text(44 + LABEL_W, y + ROW_H//2 + 4, "3 connected", fill=ACCENT, size=13))
    y += ROW_H

    # ── Version footer ─────────────────────────────────────────────────────
    y += 8
    out.append(text(24, y + 16, "Video Hotspot  v0.1.0  ·  Logos stack: Waku · Codex · Nomos",
                    fill=MUTED, size=11))

    return "\n".join(out)


# ── Compose ───────────────────────────────────────────────────────────────────

def compose(screen_fn, active_tab):
    parts = [
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{W}" height="{H}">',
        rect(0, 0, W, H, DARK),
        title_bar(),
        tab_bar(active_tab),
        screen_fn(),
        "</svg>",
    ]
    return "\n".join(parts)


# ── Main ──────────────────────────────────────────────────────────────────────

import os
out_dir = os.path.dirname(os.path.abspath(__file__))

# Map tab — active (index 0), slider at 60%
svg_map = compose(lambda: map_screen(slider_pct=0.60), active_tab=0)
with open(os.path.join(out_dir, "gui-map.svg"), "w") as f:
    f.write(svg_map)

# Upload tab — active (index 1)
svg_upload = compose(upload_screen, active_tab=1)
with open(os.path.join(out_dir, "gui-upload.svg"), "w") as f:
    f.write(svg_upload)

# Downloads tab — active (index 2)
svg_downloads = compose(downloads_screen, active_tab=2)
with open(os.path.join(out_dir, "gui-downloads.svg"), "w") as f:
    f.write(svg_downloads)

# Settings tab — active (index 3)
svg_settings = compose(settings_screen, active_tab=3)
with open(os.path.join(out_dir, "gui-settings.svg"), "w") as f:
    f.write(svg_settings)

print(f"Generated gui-map.svg, gui-upload.svg, gui-downloads.svg, gui-settings.svg  ({W}x{H})")
