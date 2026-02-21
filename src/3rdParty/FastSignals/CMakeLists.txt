cmake_minimum_required(VERSION 3.10 FATAL_ERROR)

project(FastSignals)

include(cmake/functions.cmake)

add_subdirectory(libfastsignals)

if(BUILD_FASTSIGNALS_TESTING)
    add_subdirectory(tests/libfastsignals_stress_tests)
    add_subdirectory(tests/libfastsignals_unit_tests)
endif()
