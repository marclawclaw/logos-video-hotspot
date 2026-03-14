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
# Dependencies
# ---------------------------------------------------------------------------
find_package(Qt6 REQUIRED COMPONENTS Core Concurrent Network Sql)

# NOTE: Real production build uses BLAKE3 for content hashing.
# This mock build uses Qt6's QCryptographicHash (SHA-256) to avoid a
# FetchContent build step. BLAKE3 is documented in ADR-0005.
# To enable real BLAKE3:
#   - Add BLAKE3 FetchContent or find_package
#   - Define REAL_BLAKE3 and link blake3 library
#   - Swap hash implementation in deduplicator.cpp

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
    # Include headers as sources so AUTOMOC picks up Q_OBJECT declarations
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
