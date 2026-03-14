cmake_minimum_required(VERSION 3.22)

find_package(Qt6 REQUIRED COMPONENTS Core Quick Widgets)

# Qt miniapp plugin — loaded by Basecamp (logos-app)
add_library(video_hotspot_plugin SHARED
    VideoHotspotPlugin.cpp
    VideoHotspotPlugin.h
)

target_include_directories(video_hotspot_plugin PRIVATE .)

target_link_libraries(video_hotspot_plugin
    PRIVATE
        video_hotspot_core
        Qt6::Core
        Qt6::Quick
        Qt6::Widgets
)

# Qt plugin metadata — tells logos-app the plugin type and ID
set_target_properties(video_hotspot_plugin PROPERTIES
    LIBRARY_OUTPUT_NAME "video_hotspot_plugin"
)
