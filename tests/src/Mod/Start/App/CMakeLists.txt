target_sources(
    Start_tests_run
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/FileUtilities.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/ThumbnailSource.cpp
)

target_include_directories(
    Start_tests_run
        PUBLIC
            ${CMAKE_BINARY_DIR}
)
