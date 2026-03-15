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
# Optional logos-cpp-sdk linkage
# ---------------------------------------------------------------------------

if(LOGOS_CORE_AVAILABLE)
    # Find Qt RemoteObjects (required by logos-cpp-sdk)
    find_package(Qt6 REQUIRED COMPONENTS RemoteObjects)

    # logos-cpp-sdk sources (compiled directly — no pre-built .so needed)
    target_sources(video_hotspot PRIVATE
        ${LOGOS_CPP_SDK_ROOT}/cpp/logos_api.cpp
        ${LOGOS_CPP_SDK_ROOT}/cpp/logos_api_client.cpp
        ${LOGOS_CPP_SDK_ROOT}/cpp/logos_api_consumer.cpp
        ${LOGOS_CPP_SDK_ROOT}/cpp/logos_api_provider.cpp
        ${LOGOS_CPP_SDK_ROOT}/cpp/logos_types.cpp
        ${LOGOS_CPP_SDK_ROOT}/cpp/token_manager.cpp
        ${LOGOS_CPP_SDK_ROOT}/cpp/module_proxy.cpp
    )

    target_include_directories(video_hotspot PRIVATE
        ${LOGOS_CPP_SDK_ROOT}/cpp
        ${LOGOS_LIBLOGOS_ROOT}/src/logos_core
        ${LOGOS_LIBLOGOS_ROOT}/src/common
    )

    target_link_libraries(video_hotspot PRIVATE Qt6::RemoteObjects)

    target_compile_definitions(video_hotspot PRIVATE LOGOS_CORE_AVAILABLE=1)
endif()

# ---------------------------------------------------------------------------
# Install
# ---------------------------------------------------------------------------
# logos-app plugin directory (non-portable default):
#   ~/.local/share/LogosAppNix/plugins/video_hotspot/video_hotspot.so
# logos-app plugin directory (portable):
#   ~/.local/share/LogosApp/plugins/video_hotspot/video_hotspot.so
#
# Manual install (non-portable):
#   cmake --install . --prefix ~/.local/share/LogosAppNix
#
# Or copy directly:
#   cp video_hotspot.so ~/.local/share/LogosAppNix/plugins/video_hotspot/

include(GNUInstallDirs)

install(TARGETS video_hotspot
    LIBRARY DESTINATION share/LogosAppNix/plugins/video_hotspot
    RUNTIME DESTINATION share/LogosAppNix/plugins/video_hotspot
)
