add_subdirectory(App)

target_link_libraries(Measure_tests_run
    gtest_main
    ${Google_Tests_LIBS}
    Measure
)
