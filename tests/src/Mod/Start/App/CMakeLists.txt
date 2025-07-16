add_executable(Start_tests_run
        FileUtilities.cpp
        ThumbnailSource.cpp
)

target_include_directories(
    Start_tests_run
        PUBLIC
            ${CMAKE_BINARY_DIR}
)
