
target_include_directories(Start_tests_run PUBLIC
    ${Python3_INCLUDE_DIRS}
)

target_link_libraries(Start_tests_run
    gtest_main
    ${Google_Tests_LIBS}
    Start
)

add_subdirectory(App)
