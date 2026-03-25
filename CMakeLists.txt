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

# ---------------------------------------------------------------------------
# LogosModule.cmake — use when available (Nix builds via logos-module-builder)
# ---------------------------------------------------------------------------
# When building inside `nix build` or `nix develop`, LOGOS_MODULE_BUILDER_ROOT
# is set automatically by mkLogosModule. LogosModule.cmake then handles:
#   - SDK/liblogos detection (logos_find_dependencies)
#   - Qt finding (logos_find_qt)
#   - The plugin shared-library target (logos_module)
#   - RPATH, install targets, code generation
#
# For plain `cmake -B build` without Nix, we fall back to manual SDK detection
# below so the CLI and tests still build without any Nix environment.
# ---------------------------------------------------------------------------
if(DEFINED ENV{LOGOS_MODULE_BUILDER_ROOT})
    include($ENV{LOGOS_MODULE_BUILDER_ROOT}/cmake/LogosModule.cmake)
    set(LOGOS_MODULE_BUILDER_AVAILABLE TRUE)
    message(STATUS "LogosModule.cmake: loaded from $ENV{LOGOS_MODULE_BUILDER_ROOT}")
else()
    set(LOGOS_MODULE_BUILDER_AVAILABLE FALSE)
    message(STATUS "LogosModule.cmake: not found (LOGOS_MODULE_BUILDER_ROOT not set) — using fallback SDK detection")
endif()

# ---------------------------------------------------------------------------
# Qt detection
# ---------------------------------------------------------------------------
if(LOGOS_MODULE_BUILDER_AVAILABLE)
    # logos_find_qt() is provided by LogosModule.cmake
    logos_find_qt()
else()
    find_package(Qt6 REQUIRED COMPONENTS Core Concurrent Network Sql)
endif()

# ---------------------------------------------------------------------------
# Logos SDK detection (fallback path — plain cmake without Nix)
# ---------------------------------------------------------------------------
# When LogosModule.cmake is present, logos_find_dependencies() sets
# LOGOS_CPP_SDK_ROOT and LOGOS_LIBLOGOS_ROOT and creates the imported targets.
# In the fallback path we replicate that logic manually so the CLI/tests still
# compile without any Nix environment.
# ---------------------------------------------------------------------------
if(LOGOS_MODULE_BUILDER_AVAILABLE)
    logos_find_dependencies()
    # logos_find_dependencies sets LOGOS_CPP_SDK_ROOT / LOGOS_LIBLOGOS_ROOT
    # and exposes logos_core / logos_sdk imported targets used below.
    if(LOGOS_CPP_SDK_ROOT AND LOGOS_LIBLOGOS_ROOT)
        set(LOGOS_CORE_AVAILABLE TRUE)
        message(STATUS "Logos SDK: LOGOS_CORE_AVAILABLE=1 (via LogosModule.cmake)")
    else()
        set(LOGOS_CORE_AVAILABLE FALSE)
        message(STATUS "Logos SDK: not found via logos_find_dependencies()")
    endif()
else()
    # Manual detection — search vendor/ submodules and cache overrides
    set(LOGOS_CPP_SDK_ROOT  "" CACHE PATH "Path to logos-cpp-sdk checkout")
    set(LOGOS_LIBLOGOS_ROOT "" CACHE PATH "Path to logos-liblogos checkout")
    set(LOGOS_MODULE_ROOT   "" CACHE PATH "Path to logos-module checkout")

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
        message(STATUS "Logos SDK: LOGOS_CORE_AVAILABLE=1 (vendor submodules)")
        message(STATUS "  logos-cpp-sdk:  ${_sdk_root}")
        message(STATUS "  logos-liblogos: ${_lib_root}")

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
    else()
        set(LOGOS_CORE_AVAILABLE FALSE)
        message(STATUS "Logos SDK: NOT found — SDK unavailable (CLI/tests will build in mock mode)")
        message(STATUS "  To enable: git submodule update --init --recursive && bash scripts/build-sdk.sh")
        message(STATUS "  Or use: nix develop && cmake -B build (LogosModule.cmake path)")
    endif()
endif()

# ---------------------------------------------------------------------------
# Core library (headless backend — shared by CLI, UI plugin, and tests)
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

set_target_properties(video_hotspot_core PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_link_libraries(video_hotspot_core
    PUBLIC
        Qt6::Core
        Qt6::Concurrent
        Qt6::Network
        Qt6::Sql
)

# Propagate SDK into video_hotspot_core so CLI, plugin, and tests all get it
if(LOGOS_CORE_AVAILABLE)
    target_compile_definitions(video_hotspot_core PUBLIC LOGOS_CORE_AVAILABLE=1)

    if(LOGOS_MODULE_BUILDER_AVAILABLE)
        # LogosModule.cmake exposes logos_core and logos_sdk CMake targets
        if(TARGET logos_core)
            target_link_libraries(video_hotspot_core PUBLIC logos_core)
        endif()
        if(TARGET logos_sdk)
            target_link_libraries(video_hotspot_core PUBLIC logos_sdk)
        endif()
    else()
        # Fallback: use the manually-created imported targets
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
    endif()
endif()

# ---------------------------------------------------------------------------
# UI plugin — built by logos_module() when LogosModule.cmake is available;
# otherwise falls back to add_subdirectory so plain cmake builds still work.
# ---------------------------------------------------------------------------
# logos_module() is the canonical way to produce a Logos plugin shared library.
# It handles Qt6::RemoteObjects, SDK linkage, RPATH, and install targets.
# The plugin sources are listed in module.yaml (cmake.extra_sources) which
# mkLogosModule reads; here we replicate that for the cmake-only path.
# ---------------------------------------------------------------------------
if(BUILD_UI_PLUGIN)
    if(LOGOS_MODULE_BUILDER_AVAILABLE)
        find_package(Qt6 QUIET COMPONENTS RemoteObjects)

        logos_module(
            NAME video_hotspot
            SOURCES
                ui/plugin/VideoHotspotPlugin.cpp
                ui/plugin/VideoHotspotPlugin.h
                ui/plugin/resources.qrc
            LINK_LIBRARIES
                video_hotspot_core
        )
        # Note: logos_module() creates target `video_hotspot_module_plugin`
        message(STATUS "UI plugin: built via logos_module() (LogosModule.cmake path)")
    else()
        add_subdirectory(ui/plugin)
        message(STATUS "UI plugin: built via add_subdirectory (plain cmake fallback)")
    endif()
endif()

# ---------------------------------------------------------------------------
# Standalone demo app (optional)
# ---------------------------------------------------------------------------
if(BUILD_UI_APP)
    add_subdirectory(ui/app)
endif()

# ---------------------------------------------------------------------------
# CLI frontend (headless — no Qt6::Widgets / Qt6::Quick)
# ---------------------------------------------------------------------------
add_subdirectory(cli)

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# ---------------------------------------------------------------------------
# Logos daemon runtime dependency declaration (informational)
# ---------------------------------------------------------------------------
# Runtime deps on Codex and Waku are declared in module.yaml (dependencies:)
# and in the Qt plugin metadata (ui/plugin/video_hotspot.json).
# logos-app reads these via ModuleMetadata before loading the plugin.
# No CMake action is required here — this comment is the documentation.
# ---------------------------------------------------------------------------
message(STATUS "Logos daemon runtime deps: codex, waku (declared in module.yaml + video_hotspot.json)")

configure_file(
    "${CMAKE_SOURCE_DIR}/packaging/logos-daemon-deps.txt.in"
    "${CMAKE_BINARY_DIR}/logos-daemon-deps.txt"
    @ONLY
)

install(FILES "${CMAKE_BINARY_DIR}/logos-daemon-deps.txt"
        DESTINATION share/video-hotspot)

# ---------------------------------------------------------------------------
# CPack
# ---------------------------------------------------------------------------
set(CPACK_PACKAGE_NAME "video-hotspot")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Video hotspot overlay for Logos")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "logos-daemon (>= 0.1), logos-app (>= 0.1)")
set(CPACK_RPM_PACKAGE_REQUIRES   "logos-daemon >= 0.1, logos-app >= 0.1")
include(CPack)
