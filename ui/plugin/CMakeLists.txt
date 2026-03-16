cmake_minimum_required(VERSION 3.22)

find_package(Qt6 REQUIRED COMPONENTS Core Quick Widgets QuickWidgets)

# ---------------------------------------------------------------------------
# Optional: logos-cpp-sdk integration (real Codex + Waku via LogosAPI)
# ---------------------------------------------------------------------------
# SDK detection is handled by the root CMakeLists.txt, which sets
# LOGOS_CORE_AVAILABLE and propagates SDK headers/libs through
# video_hotspot_core's PUBLIC interface. Do NOT re-detect here.
# ---------------------------------------------------------------------------

# ---------------------------------------------------------------------------
# Plugin target
# ---------------------------------------------------------------------------

add_library(video_hotspot SHARED
    VideoHotspotPlugin.cpp
    VideoHotspotPlugin.h
    resources.qrc
)

set(CMAKE_AUTORCC ON)

target_include_directories(video_hotspot PRIVATE .)

target_link_libraries(video_hotspot
    PRIVATE
        video_hotspot_core
        Qt6::Core
        Qt6::Quick
        Qt6::Widgets
        Qt6::QuickWidgets
)

# No lib prefix — logos-app loads "video_hotspot.so", not "libvideo_hotspot.so"
# See: logos-co/logos-app app/utils/LogosAppPaths.h + app/window.cpp resolvePlugin()
set_target_properties(video_hotspot PROPERTIES
    PREFIX ""
    OUTPUT_NAME "video_hotspot"
)

# ---------------------------------------------------------------------------
# SDK linkage — handled entirely by root CMakeLists.txt
# ---------------------------------------------------------------------------
# The root CMakeLists.txt detects logos-cpp-sdk and wires LOGOS_CORE_AVAILABLE,
# SDK headers, logos_sdk_lib, logos_core_lib, and Qt6::RemoteObjects into
# video_hotspot_core via its PUBLIC interface.
#
# video_hotspot inherits all of that automatically via:
#   target_link_libraries(video_hotspot PRIVATE video_hotspot_core)
#
# Do NOT re-add SDK sources via target_sources here — that would cause
# duplicate symbol linker errors (same .cpp files compiled twice into the
# final .so: once via logos_sdk_lib and once directly).
# ---------------------------------------------------------------------------

# ---------------------------------------------------------------------------
# Install
# ---------------------------------------------------------------------------
# logos-app plugin directory (non-portable default):
#   ~/.local/share/Logos/LogosAppNix/plugins/video_hotspot/video_hotspot.so
# logos-app plugin directory (portable):
#   ~/.local/share/LogosApp/plugins/video_hotspot/video_hotspot.so
#
# Manual install (non-portable):
#   cmake --install . --prefix ~/.local/share/Logos/LogosAppNix
#
# Or copy directly:
#   cp video_hotspot.so ~/.local/share/Logos/LogosAppNix/plugins/video_hotspot/

include(GNUInstallDirs)

install(TARGETS video_hotspot
    LIBRARY DESTINATION share/Logos/LogosAppNix/plugins/video_hotspot
    RUNTIME DESTINATION share/Logos/LogosAppNix/plugins/video_hotspot
)

install(FILES manifest.json
    DESTINATION share/Logos/LogosAppNix/plugins/video_hotspot
)
