# 'Google_test' is the subproject name
project(Google_tests)

# 'lib' is the folder with Google Test sources
add_subdirectory(lib)
include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

# 'Google_Tests_run' is the target name

# 'test1.cpp tests2.cpp' are source files with tests
add_executable(
    Google_Tests_run
    src/test1.cpp
    src/test2.cpp
)

target_link_libraries(Google_Tests_run gtest gtest_main)
