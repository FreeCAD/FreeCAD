target_link_libraries(Part_tests_run
    gtest_main
    ${Google_Tests_LIBS}
    Part
)

add_subdirectory(App)
