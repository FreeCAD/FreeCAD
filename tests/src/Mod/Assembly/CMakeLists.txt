
target_include_directories(Assembly_tests_run PUBLIC
    ${EIGEN3_INCLUDE_DIR}
    ${OCC_INCLUDE_DIR}
    ${Python3_INCLUDE_DIRS}
    ${XercesC_INCLUDE_DIRS}
)
target_link_directories(Assembly_tests_run PUBLIC ${OCC_LIBRARY_DIR})

target_link_libraries(Assembly_tests_run
    gtest_main
    ${Google_Tests_LIBS}
    Assembly
)

add_subdirectory(App)
