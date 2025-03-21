target_link_libraries(MeshPart_tests_run
    gtest_main
    ${Google_Tests_LIBS}
    MeshPart
)

add_subdirectory(App)
