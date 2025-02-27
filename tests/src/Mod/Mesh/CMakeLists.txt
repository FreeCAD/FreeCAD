target_link_libraries(Mesh_tests_run
    gtest_main
    ${Google_Tests_LIBS}
    Mesh
)

add_subdirectory(App)
