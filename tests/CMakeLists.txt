cmake_minimum_required(VERSION 3.22)

find_package(Qt6 REQUIRED COMPONENTS Core Test)

# ---------------------------------------------------------------------------
# Unit tests — run without a Logos node (mocked StorageClient / MessagingClient)
# ---------------------------------------------------------------------------
add_executable(test_deduplicator
    unit/test_deduplicator.cpp
)
target_link_libraries(test_deduplicator PRIVATE video_hotspot_core Qt6::Test)
add_test(NAME DeduplicatorTests COMMAND test_deduplicator)

add_executable(test_upload_queue
    unit/test_upload_queue.cpp
)
target_link_libraries(test_upload_queue PRIVATE video_hotspot_core Qt6::Test)
add_test(NAME UploadQueueTests COMMAND test_upload_queue)

add_executable(test_indexing_service
    unit/test_indexing_service.cpp
)
target_link_libraries(test_indexing_service PRIVATE video_hotspot_core Qt6::Test)
add_test(NAME IndexingServiceTests COMMAND test_indexing_service)

add_executable(test_metadata_extractor
    unit/test_metadata_extractor.cpp
)
target_link_libraries(test_metadata_extractor PRIVATE video_hotspot_core Qt6::Test)
add_test(NAME MetadataExtractorTests COMMAND test_metadata_extractor)

# ---------------------------------------------------------------------------
# E2E tests — require a running Logos testnet (CLI-driven, JSON output)
# ---------------------------------------------------------------------------
# These are shell-script driven and registered as CTest tests.
# Skipped in CI unless LOGOS_TESTNET_ENDPOINT is set.
if(DEFINED ENV{LOGOS_TESTNET_ENDPOINT})
    add_test(
        NAME E2E_UploadDownload
        COMMAND ${CMAKE_SOURCE_DIR}/tests/e2e/test_upload_download.sh
    )
    add_test(
        NAME E2E_Deduplication
        COMMAND ${CMAKE_SOURCE_DIR}/tests/e2e/test_deduplication.sh
    )
    add_test(
        NAME E2E_FolderMonitor
        COMMAND ${CMAKE_SOURCE_DIR}/tests/e2e/test_folder_monitor.sh
    )
endif()
