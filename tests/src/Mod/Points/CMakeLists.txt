target_link_libraries(Points_tests_run
    gtest_main
    ${Google_Tests_LIBS}
    Points
)

add_subdirectory(App)
