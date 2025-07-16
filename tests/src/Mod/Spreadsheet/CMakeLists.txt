add_subdirectory(App)

target_link_libraries(Spreadsheet_tests_run
    gtest_main
    ${Google_Tests_LIBS}
    Spreadsheet
)
