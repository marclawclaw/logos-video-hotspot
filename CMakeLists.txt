cmake_minimum_required(VERSION 3.22)
project(VideoHotspot VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# ---------------------------------------------------------------------------
# Build options
# ---------------------------------------------------------------------------
option(BUILD_UI_PLUGIN "Build the IComponent UI plugin for logos-app" OFF)
option(BUILD_UI_APP    "Build the standalone demo app" OFF)
option(BUILD_TESTS     "Build unit and e2e tests" ON)

set(LOGOS_CPP_SDK_ROOT  "" CACHE PATH "Path to logos-cpp-sdk checkout")
set(LOGOS_LIBLOGOS_ROOT "" CACHE PATH "Path to logos-liblogos checkout")
set(LOGOS_MODULE_ROOT   "" CACHE PATH "Path to logos-module checkout")

# ---------------------------------------------------------------------------
# Dependencies
# ---------------------------------------------------------------------------
find_package(Qt6 REQUIRED COMPONENTS Core Concurrent Network Sql)

# ---------------------------------------------------------------------------
# Core library (shared between CLI and Qt plugin)
# ---------------------------------------------------------------------------
add_library(video_hotspot_core STATIC
    core/src/storage_client.cpp
    core/src/deduplicator.cpp
    core/src/upload_queue.cpp
    core/src/indexing_service.cpp
    core/src/metadata_extractor.cpp
    core/src/messaging_client.cpp
    core/include/video_hotspot/storage_client.h
    core/include/video_hotspot/deduplicator.h
    core/include/video_hotspot/upload_queue.h
    core/include/video_hotspot/indexing_service.h
    core/include/video_hotspot/metadata_extractor.h
    core/include/video_hotspot/messaging_client.h
)

target_include_directories(video_hotspot_core
    PUBLIC  core/include
    PRIVATE core/src
)

target_link_libraries(video_hotspot_core
    PUBLIC
        Qt6::Core
        Qt6::Concurrent
        Qt6::Network
        Qt6::Sql
)

# ---------------------------------------------------------------------------
# Logos SDK detection — after video_hotspot_core so we can target_link it
# ---------------------------------------------------------------------------
# Requires pre-built SDK libs. Build with:
#   bash scripts/build-sdk.sh
#
# Or manually:
#   cd vendor/logos-module
#   cmake -B build -DCMAKE_INSTALL_PREFIX=$(pwd)/install && cmake --build build && cmake --install build
#
#   cd vendor/logos-liblogos
#   cmake -B build_vendor -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
#     -DLOGOS_CPP_SDK_ROOT=../logos-cpp-sdk \
#     -DLOGOS_MODULE_ROOT=../logos-module/install \
#     -DCMAKE_INSTALL_PREFIX=$(pwd)/install
#   cmake --build build_vendor --target logos_core logos_sdk
#   cp build_vendor/lib/liblogos_core.so build_vendor/lib/liblogos_sdk.a install/lib/
# ---------------------------------------------------------------------------

if(LOGOS_CPP_SDK_ROOT STREQUAL "")
    set(_sdk_root "${CMAKE_SOURCE_DIR}/vendor/logos-cpp-sdk")
else()
    set(_sdk_root "${LOGOS_CPP_SDK_ROOT}")
endif()
if(LOGOS_LIBLOGOS_ROOT STREQUAL "")
    set(_lib_root "${CMAKE_SOURCE_DIR}/vendor/logos-liblogos")
else()
    set(_lib_root "${LOGOS_LIBLOGOS_ROOT}")
endif()
if(LOGOS_MODULE_ROOT STREQUAL "")
    set(_mod_root "${CMAKE_SOURCE_DIR}/vendor/logos-module")
else()
    set(_mod_root "${LOGOS_MODULE_ROOT}")
endif()

set(_logos_sdk_header "${_sdk_root}/cpp/logos_api.h")
set(_logos_lib_header "${_lib_root}/src/logos_core/logos_core.h")
set(_logos_core_lib   "${_lib_root}/install/lib/liblogos_core.so")
set(_logos_sdk_lib    "${_lib_root}/install/lib/liblogos_sdk.a")
set(_logos_module_lib "${_mod_root}/install/lib/liblogos_module.a")

if(EXISTS "${_logos_sdk_header}" AND EXISTS "${_logos_lib_header}"
   AND EXISTS "${_logos_core_lib}" AND EXISTS "${_logos_sdk_lib}")
    set(LOGOS_CORE_AVAILABLE TRUE)
    message(STATUS "Logos SDK: LOGOS_CORE_AVAILABLE=1 — real SDK integration enabled")
    message(STATUS "  logos-cpp-sdk:  ${_sdk_root}")
    message(STATUS "  logos-liblogos: ${_lib_root}")

    find_package(Qt6 QUIET COMPONENTS RemoteObjects)

    # GLOBAL imported targets — visible in all subdirectories
    add_library(logos_core_lib SHARED IMPORTED GLOBAL)
    set_target_properties(logos_core_lib PROPERTIES
        IMPORTED_LOCATION "${_logos_core_lib}"
        INTERFACE_INCLUDE_DIRECTORIES "${_lib_root}/install/include;${_lib_root}/src/logos_core;${_lib_root}/src/common"
    )

    add_library(logos_sdk_lib STATIC IMPORTED GLOBAL)
    set_target_properties(logos_sdk_lib PROPERTIES
        IMPORTED_LOCATION "${_logos_sdk_lib}"
        INTERFACE_INCLUDE_DIRECTORIES "${_sdk_root}/cpp"
    )

    if(EXISTS "${_logos_module_lib}")
        add_library(logos_module_lib STATIC IMPORTED GLOBAL)
        set_target_properties(logos_module_lib PROPERTIES
            IMPORTED_LOCATION "${_logos_module_lib}"
            INTERFACE_INCLUDE_DIRECTORIES "${_mod_root}/install/include"
        )
    endif()

    # Propagate everything through video_hotspot_core PUBLIC interface
    # so CLI, UI plugin, and tests all get SDK automatically
    target_compile_definitions(video_hotspot_core PUBLIC LOGOS_CORE_AVAILABLE=1)
    target_include_directories(video_hotspot_core PUBLIC
        ${_sdk_root}/cpp
        ${_lib_root}/install/include
        ${_lib_root}/src/logos_core
        ${_lib_root}/src/common
    )
    target_link_libraries(video_hotspot_core PUBLIC
        logos_core_lib
        logos_sdk_lib
    )
    if(TARGET logos_module_lib)
        target_link_libraries(video_hotspot_core PUBLIC logos_module_lib)
    endif()
    if(Qt6RemoteObjects_FOUND)
        target_link_libraries(video_hotspot_core PUBLIC Qt6::RemoteObjects)
    endif()
else()
    set(LOGOS_CORE_AVAILABLE FALSE)
    message(STATUS "Logos SDK: NOT found — SDK unavailable")
    message(STATUS "  Expected core lib: ${_logos_core_lib}")
    message(STATUS "  Expected SDK lib:  ${_logos_sdk_lib}")
    message(STATUS "  Run: git submodule update --init --recursive && bash scripts/build-sdk.sh")
endif()

# ---------------------------------------------------------------------------
# Logos daemon runtime dependency declaration
# ---------------------------------------------------------------------------
# When LOGOS_CORE_AVAILABLE is set, the Logos daemon must be running (started
# automatically by logos-app) for SDK mode to work at runtime.
#
# Required Logos modules:
#   - Codex  ("codex"  / "CodexReplica")  — decentralised storage backend
#   - Waku   ("waku"   / "WakuReplica")   — p2p messaging transport
#
# The daemon auto-starts when logos-app launches; these modules must be loaded.
# ---------------------------------------------------------------------------
message(STATUS "Logos daemon runtime deps: Codex (codex/CodexReplica), Waku (waku/WakuReplica)")

configure_file(
    "${CMAKE_SOURCE_DIR}/packaging/logos-daemon-deps.txt.in"
    "${CMAKE_BINARY_DIR}/logos-daemon-deps.txt"
    @ONLY
)

install(FILES "${CMAKE_BINARY_DIR}/logos-daemon-deps.txt"
        DESTINATION share/video-hotspot)

# ---------------------------------------------------------------------------
# CPack — declare Logos daemon as a runtime packaging dependency
# ---------------------------------------------------------------------------
# The Logos daemon auto-starts via logos-app; no separate systemd service is
# needed — the app handles daemon lifecycle.  We only declare the package
# dependency so that package managers pull it in automatically.
# ---------------------------------------------------------------------------
set(CPACK_PACKAGE_NAME "video-hotspot")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Video hotspot overlay for Logos")

set(CPACK_DEBIAN_PACKAGE_DEPENDS "logos-daemon (>= 0.1), logos-app (>= 0.1)")
set(CPACK_RPM_PACKAGE_REQUIRES   "logos-daemon >= 0.1, logos-app >= 0.1")

include(CPack)

# ---------------------------------------------------------------------------
# CLI frontend (headless — no Qt6::Widgets / Qt6::Quick)
# ---------------------------------------------------------------------------
add_subdirectory(cli)

# ---------------------------------------------------------------------------
# Qt miniapp plugin (optional — requires logos-cpp-sdk headers)
# ---------------------------------------------------------------------------
if(BUILD_UI_PLUGIN)
    add_subdirectory(ui/plugin)
endif()

# ---------------------------------------------------------------------------
# Standalone demo app (optional)
# ---------------------------------------------------------------------------
if(BUILD_UI_APP)
    add_subdirectory(ui/app)
endif()

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
