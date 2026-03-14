cmake_minimum_required(VERSION 3.22)

find_package(Qt6 REQUIRED COMPONENTS Core Concurrent)

add_executable(video-hotspot-cli
    src/main.cpp
    src/output_formatter.cpp
    src/commands/upload_command.cpp
    src/commands/list_command.cpp
    src/commands/download_command.cpp
    src/commands/monitor_command.cpp
    src/commands/status_command.cpp
    src/commands/cache_command.cpp
)

target_link_libraries(video-hotspot-cli
    PRIVATE
        video_hotspot_core
        Qt6::Core
        Qt6::Concurrent
)

target_include_directories(video-hotspot-cli
    PRIVATE
        src
        src/commands
)

# Install symlink-friendly name
set_target_properties(video-hotspot-cli PROPERTIES OUTPUT_NAME "video-hotspot")
