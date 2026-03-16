#!/usr/bin/env bash
# build-with-nix-qt.sh — Build Video Hotspot against logos-app's Nix Qt 6.9.2
#
# logos-app ships its own Qt 6.9.2 via Nix. System Qt will NOT work — ABI and
# version mismatches cause silent crashes or link errors. You MUST build the
# plugin against the exact same Qt that logos-app uses.
#
# How to find the Nix store paths:
#   1. Look at the logos-app Nix wrapper script (e.g. `cat $(which logos-app)`)
#   2. Find the qtbase store path, e.g.:
#        /nix/store/abc123...-qtbase-6.9.2
#   3. Replace the <hash> placeholders below with the real hashes.
#
# You may need additional Qt module paths (Quick, Widgets, etc.) depending on
# your Nix profile. Separate multiple paths with semicolons.

set -euo pipefail

# ── Replace <hash> with your actual Nix store hashes ──────────────────────
NIX_QT_PREFIX="/nix/store/<hash>-qtbase-6.9.2"
# If you need additional Qt modules, append them separated by semicolons:
# NIX_QT_PREFIX="/nix/store/<hash>-qtbase-6.9.2;/nix/store/<hash>-qtdeclarative-6.9.2"

export NIX_QT_PREFIX

cmake -B build \
    -DBUILD_UI_PLUGIN=ON \
    -DBUILD_WITH_NIX_QT=ON \
    "$@"

cmake --build build --target video_hotspot
echo ""
echo "Plugin built: build/ui/plugin/video_hotspot.so"
