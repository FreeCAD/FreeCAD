
target_include_directories(PartDesign_tests_run PUBLIC
        ${EIGEN3_INCLUDE_DIR}
        ${OCC_INCLUDE_DIR}
        ${Python3_INCLUDE_DIRS}
        ${XercesC_INCLUDE_DIRS}
)
target_link_directories(PartDesign_tests_run PUBLIC ${OCC_LIBRARY_DIR})

target_link_libraries(PartDesign_tests_run
        gtest_main
        ${Google_Tests_LIBS}
        PartDesign
        Sketcher
)

add_subdirectory(App)
