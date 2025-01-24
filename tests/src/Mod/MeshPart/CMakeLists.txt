
target_include_directories(MeshPart_tests_run PUBLIC
    ${EIGEN3_INCLUDE_DIR}
    ${OCC_INCLUDE_DIR}
    ${Python3_INCLUDE_DIRS}
    ${SMESH_INCLUDE_DIR}
    ${VTK_INCLUDE_DIRS}
    ${XercesC_INCLUDE_DIRS}
)
target_link_directories(MeshPart_tests_run PUBLIC ${OCC_LIBRARY_DIR})

target_link_libraries(MeshPart_tests_run
    gtest_main
    ${Google_Tests_LIBS}
    MeshPart
)

add_subdirectory(App)
