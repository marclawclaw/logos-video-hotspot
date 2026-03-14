cmake_minimum_required(VERSION 3.22)

find_package(Qt6 REQUIRED COMPONENTS Core Quick Widgets)

# Standalone demo app — runs the miniapp without Basecamp (for development)
add_executable(video-hotspot-app
    main.cpp
)

target_link_libraries(video-hotspot-app
    PRIVATE
        video_hotspot_plugin
        Qt6::Core
        Qt6::Quick
        Qt6::Widgets
)
