add_executable(Misc_tests_run
        fmt.cpp
)

target_link_libraries(Misc_tests_run PRIVATE
    GTest::gtest_main
    GTest::gmock_main
    ${Google_Tests_LIBS}
    fmt::fmt
)
