cmake_minimum_required(VERSION 3.22)

find_package(Qt6 REQUIRED COMPONENTS Core Quick Widgets QuickWidgets)

# ---------------------------------------------------------------------------
# Optional: logos-cpp-sdk integration (real Codex + Waku via LogosAPI)
# ---------------------------------------------------------------------------
# Set LOGOS_CPP_SDK_ROOT and LOGOS_LIBLOGOS_ROOT to enable real integration.
# When not set, the plugin falls back to local filesystem/SQLite mocks.
#
# Example:
#   cmake -DLOGOS_CPP_SDK_ROOT=~/src/logos-cpp-sdk \
#         -DLOGOS_LIBLOGOS_ROOT=~/src/logos-liblogos \
#         ..
#
# Or use git submodules (after `git submodule update --init --recursive`):
#   LOGOS_CPP_SDK_ROOT defaults to vendor/logos-cpp-sdk (repo root)
#   LOGOS_LIBLOGOS_ROOT defaults to vendor/logos-liblogos (repo root)
# ---------------------------------------------------------------------------

set(_repo_root "${CMAKE_SOURCE_DIR}")

if(NOT DEFINED LOGOS_CPP_SDK_ROOT)
    set(LOGOS_CPP_SDK_ROOT "${_repo_root}/vendor/logos-cpp-sdk")
endif()
if(NOT DEFINED LOGOS_LIBLOGOS_ROOT)
    set(LOGOS_LIBLOGOS_ROOT "${_repo_root}/vendor/logos-liblogos")
endif()

set(_logos_sdk_header "${LOGOS_CPP_SDK_ROOT}/cpp/logos_api.h")
set(_logos_lib_header "${LOGOS_LIBLOGOS_ROOT}/interface.h")

if(EXISTS "${_logos_sdk_header}" AND EXISTS "${_logos_lib_header}")
    set(LOGOS_CORE_AVAILABLE TRUE)
    message(STATUS "VideoHotspotPlugin: logos-cpp-sdk found at ${LOGOS_CPP_SDK_ROOT}")
    message(STATUS "VideoHotspotPlugin: logos-liblogos found at ${LOGOS_LIBLOGOS_ROOT}")
    message(STATUS "VideoHotspotPlugin: LOGOS_CORE_AVAILABLE=1 — real Codex+Waku integration enabled")
else()
    set(LOGOS_CORE_AVAILABLE FALSE)
    message(STATUS "VideoHotspotPlugin: logos-cpp-sdk not found — using local mock")
    message(STATUS "  Run: git submodule update --init --recursive")
    message(STATUS "  Or set: -DLOGOS_CPP_SDK_ROOT=<path> -DLOGOS_LIBLOGOS_ROOT=<path>")
endif()

# ---------------------------------------------------------------------------
# Plugin target
# ---------------------------------------------------------------------------

add_library(video_hotspot SHARED
    VideoHotspotPlugin.cpp
    VideoHotspotPlugin.h
    resources.qrc
)

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
        ${LOGOS_CPP_SDK_ROOT}/cpp/token_manager.cpp
        ${LOGOS_CPP_SDK_ROOT}/cpp/module_proxy.cpp
    )

    target_include_directories(video_hotspot PRIVATE
        ${LOGOS_CPP_SDK_ROOT}/cpp
        ${LOGOS_LIBLOGOS_ROOT}
    )

    target_link_libraries(video_hotspot PRIVATE Qt6::RemoteObjects)

    target_compile_definitions(video_hotspot PRIVATE LOGOS_CORE_AVAILABLE=1)

    # Also propagate to core library so StorageClient/MessagingClient compile
    # the real integration branches
    target_compile_definitions(video_hotspot_core PUBLIC LOGOS_CORE_AVAILABLE=1)
    target_include_directories(video_hotspot_core PUBLIC
        ${LOGOS_CPP_SDK_ROOT}/cpp
        ${LOGOS_LIBLOGOS_ROOT}
    )
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
