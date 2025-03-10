target_sources(Measure_tests_run PRIVATE
        MeasureDistance.cpp
)

target_include_directories(Measure_tests_run PUBLIC
        ${CMAKE_BINARY_DIR}
)
