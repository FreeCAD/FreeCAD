add_executable(Spreadsheet_tests_run
            PropertySheet.cpp
            RenameProperty.cpp
)

target_include_directories(Spreadsheet_tests_run PUBLIC
            ${CMAKE_BINARY_DIR}
)
