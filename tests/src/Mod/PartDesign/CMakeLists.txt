add_subdirectory(App)

target_link_libraries(PartDesign_tests_run
        gtest_main
        ${Google_Tests_LIBS}
        PartDesign
        Sketcher
)
