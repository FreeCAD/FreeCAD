
target_include_directories(Sketcher_tests_run PUBLIC
    ${EIGEN3_INCLUDE_DIR}
    ${OCC_INCLUDE_DIR}
    ${Python3_INCLUDE_DIRS}
    ${XercesC_INCLUDE_DIRS}
)

target_link_libraries(Sketcher_tests_run
    gtest_main
    ${Google_Tests_LIBS}
    Sketcher
)

add_subdirectory(App)
