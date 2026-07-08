# SPDX-License-Identifier: LGPL-2.1-or-later

add_executable(Measure_tests_run
        MeasureDistance.cpp
        MeasureSnap.cpp
        MeasurePosition.cpp
)

target_include_directories(Measure_tests_run PUBLIC
        ${CMAKE_BINARY_DIR}
)

set(MeasureTestData_Files
        TestModels/PreSnapTwoCircles.FCStd
)

ADD_CUSTOM_TARGET(MeasureTestData ALL
        SOURCES ${MeasureTestData_Files}
)

fc_target_copy_resource(MeasureTestData
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_BINARY_DIR}/tests
        ${MeasureTestData_Files}
)
