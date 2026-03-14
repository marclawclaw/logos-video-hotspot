cmake_minimum_required(VERSION 3.22)

find_package(Qt6 REQUIRED COMPONENTS Core Test Concurrent Sql)

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
# E2E tests — shell-script driven, use the built video-hotspot CLI binary
# ---------------------------------------------------------------------------
# These always run (no Logos testnet required — CLI uses mock backends).

set(CLI_BIN "$<TARGET_FILE:video-hotspot-cli>")

add_test(
    NAME E2E_Upload
    COMMAND ${CMAKE_SOURCE_DIR}/tests/e2e/test_upload.sh ${CLI_BIN}
)
add_test(
    NAME E2E_Deduplication
    COMMAND ${CMAKE_SOURCE_DIR}/tests/e2e/test_deduplication.sh ${CLI_BIN}
)
add_test(
    NAME E2E_List
    COMMAND ${CMAKE_SOURCE_DIR}/tests/e2e/test_list.sh ${CLI_BIN}
)
add_test(
    NAME E2E_Download
    COMMAND ${CMAKE_SOURCE_DIR}/tests/e2e/test_download.sh ${CLI_BIN}
)
add_test(
    NAME E2E_Status
    COMMAND ${CMAKE_SOURCE_DIR}/tests/e2e/test_status.sh ${CLI_BIN}
)
add_test(
    NAME E2E_CacheClear
    COMMAND ${CMAKE_SOURCE_DIR}/tests/e2e/test_cache_clear.sh ${CLI_BIN}
)
add_test(
    NAME E2E_UploadFolder
    COMMAND ${CMAKE_SOURCE_DIR}/tests/e2e/test_upload_folder.sh ${CLI_BIN}
)

# Set test properties
set_tests_properties(
    E2E_Upload E2E_Deduplication E2E_List E2E_Download
    E2E_Status E2E_CacheClear E2E_UploadFolder
    PROPERTIES
        TIMEOUT 120
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)
