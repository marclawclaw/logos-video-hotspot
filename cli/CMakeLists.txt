cmake_minimum_required(VERSION 3.22)

find_package(Qt6 REQUIRED COMPONENTS Core)

add_executable(video-hotspot-cli
    src/main.cpp
    src/commands/upload_command.cpp
    src/commands/list_command.cpp
    src/commands/download_command.cpp
    src/commands/monitor_command.cpp
    src/commands/status_command.cpp
    src/commands/cache_command.cpp
    src/output_formatter.cpp
)

target_link_libraries(video-hotspot-cli
    PRIVATE
        video_hotspot_core
        Qt6::Core
)

target_include_directories(video-hotspot-cli PRIVATE src)
