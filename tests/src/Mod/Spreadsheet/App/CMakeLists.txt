target_sources(
    Spreadsheet_tests_run
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/PropertySheet.cpp
)

target_include_directories(
    Spreadsheet_tests_run
        PUBLIC
            ${CMAKE_BINARY_DIR}
)
