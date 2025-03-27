target_link_libraries(TechDraw_tests_run
    gtest_main
    ${Google_Tests_LIBS}
    TechDraw
)

add_subdirectory(App)
