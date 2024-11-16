target_sources(
    Measure_tests_run
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/MeasureDistance.cpp
)

target_include_directories(
    Measure_tests_run
        PUBLIC
            ${CMAKE_BINARY_DIR}
)
