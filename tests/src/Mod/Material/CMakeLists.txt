add_subdirectory(App)

target_link_libraries(Material_tests_run
    gtest_main
    ${Google_Tests_LIBS}
    Materials
)
