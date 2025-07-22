add_executable(Zipios_tests_run
        collectioncollection.cpp
        zipfile.cpp
)


target_link_libraries(Zipios_tests_run PRIVATE
    GTest::gtest_main
    GTest::gmock_main
    ${Google_Tests_LIBS}
    FreeCADApp
)

target_include_directories(
    Zipios_tests_run PRIVATE
    ${ZIPIOS_INCLUDES}
    ${ZLIB_INCLUDE_DIR}
)
