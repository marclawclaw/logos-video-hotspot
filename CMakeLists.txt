cmake_minimum_required(VERSION 3.22)
project(VideoHotspot VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# ---------------------------------------------------------------------------
# Dependencies
# ---------------------------------------------------------------------------
find_package(Qt6 REQUIRED COMPONENTS Core Concurrent Network Sql)

# BLAKE3 (deduplication hashing) — vendored as submodule or FetchContent
include(FetchContent)
FetchContent_Declare(
    blake3
    GIT_REPOSITORY https://github.com/BLAKE3-team/BLAKE3
    GIT_TAG        1.5.1
    SOURCE_SUBDIR  c
)
FetchContent_MakeAvailable(blake3)

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
)

target_include_directories(video_hotspot_core
    PUBLIC  core/include
)

target_link_libraries(video_hotspot_core
    PUBLIC
        Qt6::Core
        Qt6::Concurrent
        Qt6::Network
        Qt6::Sql
        blake3
)

# ---------------------------------------------------------------------------
# CLI frontend (headless — no Qt6::Widgets / Qt6::Quick)
# ---------------------------------------------------------------------------
add_subdirectory(cli)

# ---------------------------------------------------------------------------
# Qt miniapp plugin
# ---------------------------------------------------------------------------
add_subdirectory(ui/plugin)

# ---------------------------------------------------------------------------
# Standalone demo app (for development without Basecamp)
# ---------------------------------------------------------------------------
add_subdirectory(ui/app)

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------
enable_testing()
add_subdirectory(tests)
