
target_include_directories(Mesh_tests_run PUBLIC
    ${EIGEN3_INCLUDE_DIR}
    ${OCC_INCLUDE_DIR}
    ${Python3_INCLUDE_DIRS}
    ${XercesC_INCLUDE_DIRS}
)

target_link_libraries(Mesh_tests_run
    gtest_main
    ${Google_Tests_LIBS}
    Mesh
)

add_subdirectory(App)
