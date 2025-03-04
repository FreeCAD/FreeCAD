target_sources(Spreadsheet_tests_run PRIVATE
            PropertySheet.cpp
)

target_include_directories(Spreadsheet_tests_run PUBLIC
            ${CMAKE_BINARY_DIR}
)
