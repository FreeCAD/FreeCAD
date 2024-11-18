
target_include_directories(Part_tests_run PUBLIC
    ${EIGEN3_INCLUDE_DIR}
    ${OCC_INCLUDE_DIR}
    ${Python3_INCLUDE_DIRS}
    ${XercesC_INCLUDE_DIRS}
)

target_link_libraries(Part_tests_run
    gtest_main
    ${Google_Tests_LIBS}
    Part
)

add_subdirectory(App)
