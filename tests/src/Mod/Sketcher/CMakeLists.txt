add_subdirectory(App)

target_link_libraries(Sketcher_tests_run
    gtest_main
    ${Google_Tests_LIBS}
    Sketcher
)
