#!/usr/bin/env bash
# build-sdk.sh — Build logos-cpp-sdk, logos-module, and logos-liblogos
# into vendor/*/install/ so CMake can find them.
#
# Prerequisites:
#   sudo apt install qt6-remoteobjects-dev libqt6remoteobjects6
#   git submodule update --init --recursive
#
# Usage:
#   bash scripts/build-sdk.sh

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SDK_ROOT="$REPO_ROOT/vendor/logos-cpp-sdk"
LIBLOGOS_ROOT="$REPO_ROOT/vendor/logos-liblogos"
MODULE_ROOT="$REPO_ROOT/vendor/logos-module"

echo "=== build-sdk.sh: building Logos SDK dependencies ==="
echo "Repo:     $REPO_ROOT"
echo "SDK:      $SDK_ROOT"
echo "liblogos: $LIBLOGOS_ROOT"
echo "module:   $MODULE_ROOT"
echo ""

# ── logos-module ─────────────────────────────────────────────────────────────
echo ">>> Building logos-module..."
cd "$MODULE_ROOT"
cmake -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX="$(pwd)/install" \
  -DCMAKE_POSITION_INDEPENDENT_CODE=ON
cmake --build build -j"$(nproc)"
cmake --install build
echo "logos-module: done"
echo ""

# ── logos-liblogos (logos_core + logos_sdk) ──────────────────────────────────
echo ">>> Building logos-liblogos..."
cd "$LIBLOGOS_ROOT"
cmake -B build_vendor \
  -DCMAKE_BUILD_TYPE=Release \
  -DLOGOS_CPP_SDK_ROOT="$SDK_ROOT" \
  -DLOGOS_MODULE_ROOT="$MODULE_ROOT/install" \
  -DCMAKE_POSITION_INDEPENDENT_CODE=ON
cmake --build build_vendor --target logos_core logos_sdk -j"$(nproc)"

# Install headers and libs manually (install target also installs googletest which may fail)
mkdir -p install/lib install/include
cp build_vendor/lib/liblogos_core.so install/lib/
cp build_vendor/lib/liblogos_sdk.a   install/lib/
cp "$SDK_ROOT/cpp/logos_api.h"          install/include/
cp "$SDK_ROOT/cpp/logos_api_client.h"   install/include/
cp "$SDK_ROOT/cpp/logos_api_consumer.h" install/include/
cp "$SDK_ROOT/cpp/logos_api_provider.h" install/include/
cp "$SDK_ROOT/cpp/logos_types.h"        install/include/
cp "$SDK_ROOT/cpp/logos_mode.h"         install/include/
cp "$SDK_ROOT/cpp/module_proxy.h"       install/include/
cp src/logos_core/logos_core.h          install/include/
echo "logos-liblogos: done"
echo ""

echo "=== build-sdk.sh: all done ==="
echo "Now run: cmake -B build && cmake --build build"
