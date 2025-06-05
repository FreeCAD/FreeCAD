cmake_minimum_required(VERSION 3.8 FATAL_ERROR)

project(FastSignals)

# Use `-DBUILD_TESTING=OFF` to disable testing/benchmarking.
enable_testing()

file(GLOB LIBFASTSIGNALS_SRC "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")
add_library(libfastsignals ${LIBFASTSIGNALS_SRC})
set_property(TARGET libfastsignals PROPERTY POSITION_INDEPENDENT_CODE ON)
target_include_directories(libfastsignals INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")

if(BUILD_TESTING)
    # add_subdirectory(tests/benchmark)
    # add_subdirectory(tests/libfastsignals_bench)
    #add_subdirectory(tests/libfastsignals_stress_tests)
    add_subdirectory(tests/libfastsignals_unit_tests)
endif()
