
target_include_directories(Measure_tests_run PUBLIC
    ${EIGEN3_INCLUDE_DIR}
    ${OCC_INCLUDE_DIR}
    ${Python3_INCLUDE_DIRS}
    ${SMESH_INCLUDE_DIR}
    ${VTK_INCLUDE_DIRS}
    ${XercesC_INCLUDE_DIRS}
)

target_link_libraries(Measure_tests_run
    gtest_main
    ${Google_Tests_LIBS}
    Measure
)

add_subdirectory(App)
