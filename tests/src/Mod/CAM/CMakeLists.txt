add_subdirectory(App)

target_link_libraries(CAM_tests_run
    GTest::gtest_main
    Path
)
